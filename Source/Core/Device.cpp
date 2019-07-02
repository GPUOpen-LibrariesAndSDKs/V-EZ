//
// Copyright (c) 2018 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#include <cmath>
#include <iostream>
#include <array>
#include <unordered_map>
#include <algorithm>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include "Utility/VkHelpers.h"
#include "Utility/SpinLock.h"
#include "Utility/ThreadPool.h"
#include "Utility/ObjectLookup.h"
#include "Instance.h"
#include "PhysicalDevice.h"
#include "Device.h"
#include "Queue.h"
#include "Swapchain.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "SyncPrimitivesPool.h"
#include "PipelineCache.h"
#include "DescriptorSetLayoutCache.h"
#include "RenderPassCache.h"
#include "Buffer.h"
#include "Image.h"
#include "ImageView.h"
#include "Framebuffer.h"
#include "Fence.h"

namespace vez
{
    static std::unordered_map<VezMemoryFlags, VmaMemoryUsage> memoryFlagsToVmaMemoryUsage = {
        { VEZ_MEMORY_GPU_ONLY, VMA_MEMORY_USAGE_GPU_ONLY },
        { VEZ_MEMORY_CPU_ONLY, VMA_MEMORY_USAGE_CPU_ONLY },
        { VEZ_MEMORY_CPU_TO_GPU, VMA_MEMORY_USAGE_CPU_TO_GPU },
        { VEZ_MEMORY_GPU_TO_CPU, VMA_MEMORY_USAGE_GPU_TO_CPU },
    };

    static std::unordered_map<VezMemoryFlags, VmaAllocationCreateFlagBits> memoryFlagsToVmaMemoryCreateFlag = {
        { 0, static_cast<VmaAllocationCreateFlagBits>(0) },
        { VEZ_MEMORY_DEDICATED_ALLOCATION, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT }
    };

    VkResult Device::Create(PhysicalDevice* pPhysicalDevice, const VezDeviceCreateInfo* pCreateInfo, Device** ppDevice)
    {
        // Enumerate queue families.
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(pPhysicalDevice->GetHandle(), &queueFamilyCount, nullptr);
            
        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(pPhysicalDevice->GetHandle(), &queueFamilyCount, queueFamilyProperties.data());

        // Allocate handles for all available queues.
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(queueFamilyCount);
        std::vector<std::vector<float>> priorities(queueFamilyCount);
        for (auto i = 0U; i < queueFamilyCount; ++i)
        {
            const float defaultPriority = 1.0f;
            priorities[i].resize(queueFamilyProperties[i].queueCount, defaultPriority);
            queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfos[i].queueFamilyIndex = i;
            queueCreateInfos[i].queueCount = queueFamilyProperties[i].queueCount;
            queueCreateInfos[i].pQueuePriorities = priorities[i].data();
        }

        // Enable all physical device available features.
        VkPhysicalDeviceFeatures enabledFeatures = {};
        vkGetPhysicalDeviceFeatures(pPhysicalDevice->GetHandle(), &enabledFeatures);

        // Create the Vulkan device.
        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pNext = pCreateInfo->pNext;
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
        deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
        deviceCreateInfo.enabledLayerCount = pCreateInfo->enabledLayerCount;
        deviceCreateInfo.ppEnabledLayerNames = pCreateInfo->ppEnabledLayerNames;
        deviceCreateInfo.enabledExtensionCount = pCreateInfo->enabledExtensionCount;
        deviceCreateInfo.ppEnabledExtensionNames = pCreateInfo->ppEnabledExtensionNames;

        VkDevice handle = VK_NULL_HANDLE;
        auto result = vkCreateDevice(pPhysicalDevice->GetHandle(), &deviceCreateInfo, nullptr, &handle);
        if (result != VK_SUCCESS)
            return result;

        // Initialize Device class.
        auto device = new Device;
        memcpy(&device->m_createInfo, pCreateInfo, sizeof(VezDeviceCreateInfo));
        device->m_physicalDevice = pPhysicalDevice;
        device->m_handle = handle;
        device->m_syncPrimitivesPool = new SyncPrimitivesPool(device);
        device->m_pipelineCache = new PipelineCache(device);
        device->m_descriptorSetLayoutCache = new DescriptorSetLayoutCache(device);
        device->m_renderPassCache = new RenderPassCache(device);        

        // Get handles to all of the previously enumerated and created queues.
        device->m_queues.resize(queueFamilyCount);        
        for (auto i = 0U; i < queueFamilyCount; ++i)
        {
            // Get all of the queues for the given family.
            QueueFamily qf;
            for (auto j = 0U; j < queueCreateInfos[i].queueCount; ++j)
            {
                VkQueue queue = VK_NULL_HANDLE;
                vkGetDeviceQueue(handle, i, j, &queue);
                if (queue)
                    device->m_queues[i].push_back(new Queue(device, queue, i, j, queueFamilyProperties[i]));
            }
        }

        // Initialize memory allocator.
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = pPhysicalDevice->GetHandle();
        allocatorInfo.device = handle;
        result = vmaCreateAllocator(&allocatorInfo, &device->m_memAllocator);
        if (result != VK_SUCCESS)
        {
            delete device;
            return result;
        }

        // Allocate an 8MB pinned memory buffer to use for efficient data transfers.
        VezBufferCreateInfo bufferCreateInfo = {};
        bufferCreateInfo.size = MEGABYTES(128);
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        result = device->CreateBuffer(VEZ_MEMORY_CPU_ONLY | VEZ_MEMORY_DEDICATED_ALLOCATION, &bufferCreateInfo, &device->m_pinnedMemoryBuffer);
        if (result != VK_SUCCESS)
        {
            delete device;
            return result;
        }

        // Keep the pinned memory mapped for the lifetime of the device instance.
        result = device->MapBuffer(device->m_pinnedMemoryBuffer, 0, bufferCreateInfo.size, &device->m_pinnedMemoryPtr);
        if (result != VK_SUCCESS)
        {
            delete device;
            return result;
        }

        // Copy address of object instance.
        *ppDevice = device;

        // Return success.
        return VK_SUCCESS;
    }

    void Device::Destroy(Device* pDevice)
    {
        delete pDevice;
    }

    Queue* Device::GetQueue(uint32_t queueFamilyIndex, uint32_t queueIndex)
    {
        // Check queue family index range.
        if (queueFamilyIndex >= m_queues.size())
            return nullptr;

        // Check index within queue family.
        if (queueIndex >= m_queues[queueFamilyIndex].size())
            return nullptr;

        // Return pointer to queue.
        return m_queues[queueFamilyIndex][queueIndex];
    }

    Queue* Device::GetQueueByFlags(VkQueueFlags queueFlags, uint32_t queueIndex)
    {
        // Enumerate all available queues.
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice->GetHandle(), &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice->GetHandle(), &queueFamilyCount, queueFamilyProperties.data());

        // Iterate over queues in order to find one matching requested flags.
        // Favor queue families matching only what's specified in queueFlags over families having other bits set as well.
        VkQueueFlags minFlags = ~0;
        Queue* bestQueue = nullptr;
        for (auto queueFamilyIndex = 0U; queueFamilyIndex < queueFamilyCount; ++queueFamilyIndex)
        {
            if (((queueFamilyProperties[queueFamilyIndex].queueFlags & queueFlags) == queueFlags) && queueIndex < queueFamilyProperties[queueFamilyIndex].queueCount)
            {
                if (queueFamilyProperties[queueFamilyIndex].queueFlags < minFlags)
                {
                    minFlags = queueFamilyProperties[queueFamilyIndex].queueFlags;
                    bestQueue = m_queues[queueFamilyIndex][queueIndex];
                }
            }
        }

        // Return the queue for the given flags.
        return bestQueue;
    }

    CommandPool* Device::GetCommandPool(Queue* queue)
    {
        CommandPool* pool = nullptr;

        // Make thread-safe.
        BEGIN_THREAD_SAFE_BLOCK();

        // Find QueueCommandPools for current thread.
        auto itr = m_commandPools.find(std::this_thread::get_id());
        if (itr != m_commandPools.end())
        {
            // Find CommandPool for specified queue.
            auto& queueCommandPools = itr->second;
            auto itr2 = queueCommandPools.find(queue);
            if (itr2 != queueCommandPools.end())
            {
                pool = itr2->second;
            }
            else
            {
                // Create a new CommandPool for the specified queue.
                if (CommandPool::Create(this, queue->GetFamilyIndex(), &pool) == VK_SUCCESS)
                    queueCommandPools.emplace(queue, pool);
            }
        }
        else
        {
            // Create a new QueueCommandPools entry.
            QueueCommandPools queueCommandPools;

            // Create a new CommandPool for the specified queue.
            if (CommandPool::Create(this, queue->GetFamilyIndex(), &pool) == VK_SUCCESS)
            {
                // Add new CommandPool to the new QueueCommandPools instance.
                queueCommandPools.emplace(queue, pool);

                // Add the QueueCommandPools instance to the device level map keyed by the threadID.
                m_commandPools.emplace(std::this_thread::get_id(), std::move(queueCommandPools));
            }
        }

        // End thread-safe block.
        END_THREAD_SAFE_BLOCK();

        // Return result.
        return pool;
    }

    VkResult Device::AllocateCommandBuffers(Queue* pQueue, const void* pNext, uint32_t commandBufferCount, CommandBuffer** ppCommandBuffers, uint64_t streamEncoderBlockSize)
    {
        // Get or create a CommandPool for the given queue and threadID.
        CommandPool* pool = GetCommandPool(pQueue);

        // Allocate object handles.
        std::vector<VkCommandBuffer> handles(commandBufferCount);
        auto result = pool->AllocateCommandBuffers(pNext, commandBufferCount, handles.data());
        if (result != VK_SUCCESS)
            return result;

        // Wrap handles with CommandBuffer class instances.
        for (auto i = 0U; i < commandBufferCount; ++i)
            ppCommandBuffers[i] = new CommandBuffer(pool, handles[i], streamEncoderBlockSize);

        // Return success.
        return VK_SUCCESS;
    }

    void Device::FreeCommandBuffers(uint32_t commandBufferCount, CommandBuffer** ppCommandBuffers)
    {
        // Destroy all of the command buffers.
        for (auto i = 0U; i < commandBufferCount; ++i)
            delete ppCommandBuffers[i];
    }

    VkResult Device::WaitIdle()
    {
        return vkDeviceWaitIdle(m_handle);
    }

    VkResult Device::SetVSync(VkBool32 enabled)
    {
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    VkResult Device::CreateBuffer(VezMemoryFlags memFlags, const VezBufferCreateInfo* pCreateInfo, Buffer** ppBuffer)
    {
        // Create Vulkan buffer handle.
        VkBufferCreateInfo bufferCreateInfo = {};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.pNext = pCreateInfo->pNext;
        bufferCreateInfo.usage = pCreateInfo->usage;
        bufferCreateInfo.size = pCreateInfo->size;

        // If no queue family indices are passed in with pCreateInfo, then assume Buffer will be used
        // by all available queues and hence require VK_SHARING_MODE_CONCURRENT.
        std::vector<uint32_t> queueFamilyIndices;
        if (pCreateInfo->queueFamilyIndexCount == 0)
        {
            // Generate a vector of the queue family indices.
            queueFamilyIndices.resize(m_queues.size());
            for (auto i = 0U; i < queueFamilyIndices.size(); ++i)
                queueFamilyIndices[i] = i;

            // Set sharing mode to concurrent since Buffer is accessible across all queues.
            bufferCreateInfo.sharingMode = (queueFamilyIndices.size() > 1) ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
            bufferCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
            bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
        }
        else
        {
            // If only a single queue can access the Buffer then sharing is exclusive.
            // Else if more than one queue will access the Buffer then sharing is concurrent.
            bufferCreateInfo.sharingMode = (pCreateInfo->queueFamilyIndexCount == 1) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
            bufferCreateInfo.queueFamilyIndexCount = pCreateInfo->queueFamilyIndexCount;
            bufferCreateInfo.pQueueFamilyIndices = pCreateInfo->pQueueFamilyIndices;
        }

        // Allocate memory from the Vulkan Memory Allocator.
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkResult result = VK_SUCCESS;
        if ((memFlags & VEZ_MEMORY_NO_ALLOCATION) == 0)
        {
            // Determine appropriate memory usage flags.
            VmaAllocationCreateInfo allocCreateInfo = {};
            allocCreateInfo.usage = memoryFlagsToVmaMemoryUsage.at(memFlags & (VEZ_MEMORY_DEDICATED_ALLOCATION - 1U));
            allocCreateInfo.flags = memoryFlagsToVmaMemoryCreateFlag.at(memFlags & ~(VEZ_MEMORY_DEDICATED_ALLOCATION - 1U));

            result = vmaCreateBuffer(m_memAllocator, &bufferCreateInfo, &allocCreateInfo, &buffer, &allocation, nullptr);
        }
        else
        {
            result = vkCreateBuffer(m_handle, &bufferCreateInfo, nullptr, &buffer);
        }

        // Create an Buffer class instance from handle.
        *ppBuffer = Buffer::CreateFromHandle(this, pCreateInfo, buffer, allocation);

        // Return success.
        return VK_SUCCESS;
    }

    void Device::DestroyBuffer(Buffer* pBuffer)
    {
        if (pBuffer->GetAllocation() != VK_NULL_HANDLE)
            vmaDestroyBuffer(m_memAllocator, pBuffer->GetHandle(), pBuffer->GetAllocation());
        else
            vkDestroyBuffer(m_handle, pBuffer->GetHandle(), nullptr);

        delete pBuffer;
    }

    VkResult Device::BufferSubData(Buffer* pBuffer, VkDeviceSize offset, VkDeviceSize size, const void* pData)
    {
        // Get a one-time submit command buffer.
        auto cmdBuffer = GetOneTimeSubmitCommandBuffer();

        // Get the queue family index the command buffer was created for.
        auto queue = m_queues[cmdBuffer->GetPool()->GetQueueFamilyIndex()][0];

        // Get required alignment flush size for selected physical device.
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(m_physicalDevice->GetHandle(), &properties);
        auto alignedFlushSize = properties.limits.nonCoherentAtomSize;

        // Copy the buffer data to the device in blocks based on size of pinned memory buffer.
        auto bytesRemaining = size;
        auto dstOffset = offset;
        auto srcOffset = 0ULL;
        while (bytesRemaining)
        {
            // Determine total byte size to copy this iteration.
            auto bytesToCopy = std::min(m_pinnedMemoryBuffer->GetCreateInfo().size, bytesRemaining);

            // Copy the host memory to the pinned memory buffer.
            memcpy(m_pinnedMemoryPtr, &reinterpret_cast<const uint8_t*>(pData)[srcOffset], bytesToCopy);

            // Flush must be aligned according to physical device's limits.
            auto alignedBytesToCopy = static_cast<VkDeviceSize>(std::ceil(bytesToCopy / static_cast<float>(alignedFlushSize))) * alignedFlushSize;

            // Flush the memory write.
            VmaAllocationInfo allocInfo = {};
            vmaGetAllocationInfo(m_memAllocator, m_pinnedMemoryBuffer->GetAllocation(), &allocInfo);

            VkMappedMemoryRange memoryRange = {};
            memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            memoryRange.memory = allocInfo.deviceMemory;
            memoryRange.offset = 0;
            memoryRange.size = alignedBytesToCopy;
            vkFlushMappedMemoryRanges(m_handle, 1, &memoryRange);

            // Copy the pinned memory buffer to the destination buffer.
            VezBufferCopy region = {};
            region.srcOffset = 0;
            region.dstOffset = dstOffset;
            region.size = bytesToCopy;
            cmdBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            cmdBuffer->CmdCopyBuffer(m_pinnedMemoryBuffer, pBuffer, 1, &region);
            cmdBuffer->End();

            // Submit to the queue.
            auto cmdBufferHandle = cmdBuffer->GetHandle();

            VezSubmitInfo submitInfo = {};
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cmdBufferHandle;
            auto result = queue->Submit(1, &submitInfo, nullptr);
            if (result != VK_SUCCESS)
                return result;

            // Wait for all operations to complete.
            queue->WaitIdle();

            // Update running counters.
            bytesRemaining -= bytesToCopy;
            dstOffset += bytesToCopy;
            srcOffset += bytesToCopy;
        }

        // Return success.
        return VK_SUCCESS;
    }

    VkResult Device::MapBuffer(Buffer* pBuffer, VkDeviceSize offset, VkDeviceSize size, void** ppData)
    {
        return vmaMapMemory(m_memAllocator, pBuffer->GetAllocation(), ppData);
    }

    void Device::UnmapBuffer(Buffer* pBuffer)
    {
        vmaUnmapMemory(m_memAllocator, pBuffer->GetAllocation());
    }

    VkResult Device::FlushMappedBufferRanges(uint32_t bufferRangeCount, const VezMappedBufferRange* pBufferRanges)
    {
        std::vector<VkMappedMemoryRange> memoryRanges(bufferRangeCount);
        for (auto i = 0U; i < bufferRangeCount; ++i)
        {
            // Lookup Buffer object handle.
            auto bufferImpl = ObjectLookup::GetImplBuffer(pBufferRanges[i].buffer);
            if (!bufferImpl)
                return VK_INCOMPLETE;

            VmaAllocationInfo allocInfo = {};
            vmaGetAllocationInfo(m_memAllocator, bufferImpl->GetAllocation(), &allocInfo);

            memoryRanges[i].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            memoryRanges[i].memory = allocInfo.deviceMemory;
            memoryRanges[i].offset = allocInfo.offset + pBufferRanges[i].offset;
            memoryRanges[i].size = pBufferRanges[i].size;
        }

        return vkFlushMappedMemoryRanges(m_handle, static_cast<uint32_t>(memoryRanges.size()), memoryRanges.data());
    }

    VkResult Device::InvalidateMappedBufferRanges(uint32_t bufferRangeCount, const VezMappedBufferRange* pBufferRanges)
    {
        std::vector<VkMappedMemoryRange> memoryRanges(bufferRangeCount);
        for (auto i = 0U; i < bufferRangeCount; ++i)
        {
            // Lookup Buffer object handle.
            auto bufferImpl = ObjectLookup::GetImplBuffer(pBufferRanges[i].buffer);
            if (!bufferImpl)
                return VK_INCOMPLETE;

            VmaAllocationInfo allocInfo = {};
            vmaGetAllocationInfo(m_memAllocator, bufferImpl->GetAllocation(), &allocInfo);

            memoryRanges[i].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            memoryRanges[i].memory = allocInfo.deviceMemory;
            memoryRanges[i].offset = allocInfo.offset + pBufferRanges[i].offset;
            memoryRanges[i].size = pBufferRanges[i].size;
        }

        return vkInvalidateMappedMemoryRanges(m_handle, static_cast<uint32_t>(memoryRanges.size()), memoryRanges.data());
    }

    VkResult Device::CreateImage(VezMemoryFlags memFlags, const VezImageCreateInfo* pCreateInfo, Image** ppImage)
    {
        // Create Vulkan image handle.
        VkImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.pNext = pCreateInfo->pNext;
        imageCreateInfo.flags = pCreateInfo->flags;
        imageCreateInfo.imageType = pCreateInfo->imageType;
        imageCreateInfo.format = pCreateInfo->format;
        memcpy(&imageCreateInfo.extent, &pCreateInfo->extent, sizeof(VkExtent3D));
        imageCreateInfo.mipLevels = pCreateInfo->mipLevels;
        imageCreateInfo.arrayLayers = pCreateInfo->arrayLayers;
        imageCreateInfo.samples = pCreateInfo->samples;
        imageCreateInfo.tiling = pCreateInfo->tiling;
        imageCreateInfo.usage = pCreateInfo->usage;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        // If no queue family indices are passed in with pCreateInfo, then assume Image will be used
        // by all available queues and hence require VK_SHARING_MODE_CONCURRENT.
        std::vector<uint32_t> queueFamilyIndices;
        if (pCreateInfo->queueFamilyIndexCount == 0)
        {
            // Generate a vector of the queue family indices.
            queueFamilyIndices.resize(m_queues.size());
            for (auto i = 0U; i < queueFamilyIndices.size(); ++i)
                queueFamilyIndices[i] = i;

            // Set sharing mode to concurrent since Image is accessible across all queues.
            imageCreateInfo.sharingMode = (queueFamilyIndices.size() == 1) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
            imageCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
            imageCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
        }
        else
        {
            // If only a single queue can access the Image then sharing is exclusive.
            // Else if more than one queue will access the Image then sharing is concurrent.
            imageCreateInfo.sharingMode = (pCreateInfo->queueFamilyIndexCount == 1) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
            imageCreateInfo.queueFamilyIndexCount = pCreateInfo->queueFamilyIndexCount;
            imageCreateInfo.pQueueFamilyIndices = pCreateInfo->pQueueFamilyIndices;
        }        

        // Allocate memory using the Vulkan Memory Allocator (unless memFlags has the NO_ALLOCATION bit set).
        VmaAllocation allocation = VK_NULL_HANDLE;
        VkImage handle = VK_NULL_HANDLE;

        VkResult result = VK_SUCCESS;
        if ((memFlags & VEZ_MEMORY_NO_ALLOCATION) == 0)
        {
            // Determine appropriate memory usage flags.
            VmaAllocationCreateInfo allocCreateInfo = {};
            allocCreateInfo.usage = memoryFlagsToVmaMemoryUsage.at(memFlags & (VEZ_MEMORY_DEDICATED_ALLOCATION - 1U));
            allocCreateInfo.flags = memoryFlagsToVmaMemoryCreateFlag.at(memFlags & ~(VEZ_MEMORY_DEDICATED_ALLOCATION - 1U));

            result = vmaCreateImage(m_memAllocator, &imageCreateInfo, &allocCreateInfo, &handle, &allocation, nullptr);
        }
        else
        {
            result = vkCreateImage(m_handle, &imageCreateInfo, nullptr, &handle);
        }

        // Determine a "default" image layout based on the usage.  This default image layout will be assumed during command buffer recording.
        // Ordering of conditional statements below determine image layout precedence.
        // Example: When usage is SAMPLED_BIT, that takes precendence over the image being used as a color attachment or for transfer operations.
        auto usage = pCreateInfo->usage;
        VkImageLayout defaultLayout = imageCreateInfo.initialLayout;
        if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) defaultLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        else if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) defaultLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        else if (usage & VK_IMAGE_USAGE_STORAGE_BIT) defaultLayout = VK_IMAGE_LAYOUT_GENERAL;
        else if (usage & VK_IMAGE_USAGE_SAMPLED_BIT) defaultLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        else if (usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) defaultLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        else if (usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) defaultLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        else if (usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) defaultLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        // Create an Image class instance from handle.
        *ppImage = Image::CreateFromHandle(this, pCreateInfo, defaultLayout, handle, allocation);

        // Transition image to a default layout.
        TransitionImageLayout(*ppImage, imageCreateInfo.initialLayout, defaultLayout);

        // Return success.
        return VK_SUCCESS;
    }

    void Device::DestroyImage(Image* pImage)
    {
        if (pImage->GetAllocation() != VK_NULL_HANDLE)
            vmaDestroyImage(m_memAllocator, pImage->GetHandle(), pImage->GetAllocation());
        else
            vkDestroyImage(m_handle, pImage->GetHandle(), nullptr);

        delete pImage;
    }

    VkResult Device::ImageSubData(Image* pImage, const VezImageSubDataInfo* pSubDataInfo, const void* pData)
    {
        // Handle compressed and uncompressed image formats separately.
        if (!IsCompressedImageFormat(pImage->GetCreateInfo().format))
            return UncompressedImageSubData(pImage, pSubDataInfo, pData);
        else
            return CompressedImageSubData(pImage, pSubDataInfo, pData);
    }

    void Device::TransitionImageLayout(Image* pImage, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        // Get the one-time submit command buffer instance created by the device.
        CommandBuffer* cmdBuffer = GetOneTimeSubmitCommandBuffer();

        // Fill in the memory barrier info.
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = pImage->GetHandle();

        // All mip levels and array layers will be transitioned.
        const auto& imageCreateInfo = pImage->GetCreateInfo();
        barrier.subresourceRange.aspectMask = GetImageAspectFlags(imageCreateInfo.format);
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = imageCreateInfo.mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = imageCreateInfo.arrayLayers;

        // Executive the native Vulkan call.
        cmdBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        vkCmdPipelineBarrier(cmdBuffer->GetHandle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        cmdBuffer->End();

        // Get the queue family index the command buffer was created for.
        auto queue = m_queues[cmdBuffer->GetPool()->GetQueueFamilyIndex()][0];

        // Submit to the queue.
        auto cmdBufferHandle = cmdBuffer->GetHandle();

        VezSubmitInfo submitInfo = {};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBufferHandle;
        queue->Submit(1, &submitInfo, nullptr);

        // Wait for all operations to complete before returning.
        queue->WaitIdle();
    }

    void Device::QueueSubmission(Fence* pFence)
    {
        // Store references to all semaphores using fence as key so they can be released when fence is destroyed.
        if (pFence)
        {
            m_trackedFencesLock.Lock();
            m_trackedFences.push(pFence);
            m_trackedFencesLock.Unlock();
        }

        // Track the number of fences that have been created for queue submissions.
        ++m_fencesQueuedRunningCount;

        // Evaluate tracked fences after a certain threshold has been reached.
        if (m_fencesQueuedRunningCount % m_fencesQueuedUntilTrackedFencesEval == 0)
        {
            m_trackedFencesLock.Lock();
            while (m_trackedFences.size() > 0)
            {
                auto fenceImpl = m_trackedFences.front();
                auto result = vkGetFenceStatus(m_handle, fenceImpl->GetHandle());
                if (result == VK_SUCCESS)
                {
                    DestroyFence(fenceImpl);
                    m_trackedFences.pop();
                }
                else
                {
                    break;
                }
            }
            m_trackedFencesLock.Unlock();
        }

        // Evaluate RenderPass class objects no longer in use after a certain threshold has been reached.
        if (m_fencesQueuedRunningCount % m_fencesQueuedUntilRenderPassCacheEval == 0)
        {
            m_renderPassCache->DestroyUnusedRenderPasses();
        }
    }

    void Device::DestroyFence(Fence* pFence)
    {
        // Remove from ObjectLookup cache.
        ObjectLookup::RemoveImplFence(pFence->GetHandle());

        // Return all semaphores to the sync primitives pool.
        const auto& semaphores = pFence->GetSemaphores();
        GetSyncPrimitivesPool()->ReleaseSemaphores(static_cast<uint32_t>(semaphores.size()), semaphores.data());

        // Return fence to sync primitives pool.
        GetSyncPrimitivesPool()->ReleaseFence(pFence->GetHandle());

        // Delete class object.
        delete pFence;
    }

    void Device::DestroySemaphore(VkSemaphore semaphore)
    {
        // Return semaphore to sync primitives pool.
        GetSyncPrimitivesPool()->ReleaseSemaphores(1, &semaphore);
    }

    Device::~Device()
    {
        // Destroy pinned memory buffer.
        if (m_pinnedMemoryBuffer)
        {
            UnmapBuffer(m_pinnedMemoryBuffer);
            DestroyBuffer(m_pinnedMemoryBuffer);
        }

        // Destroy one time submit command buffers.
        for (auto it : m_oneTimeSubmitCommandBuffers)
            FreeCommandBuffers(1, &it.second);

        // Destroy sync primitives pool.
        if (m_syncPrimitivesPool)
            delete m_syncPrimitivesPool;

        // Destroy pipeline cache.
        if (m_pipelineCache)
            delete m_pipelineCache;

        // Destroy descriptor set layout cache.
        if (m_descriptorSetLayoutCache)
            delete m_descriptorSetLayoutCache;

        // Destroy render pass cache.
        if (m_renderPassCache)
            delete m_renderPassCache;

        // Destroy per queue command pools.
        for (auto it : m_commandPools)
        {
            for (auto it2 : it.second)
            {
                delete it2.second;
            }
        }

        // Destroy memory allocator.
        if (m_memAllocator)
            vmaDestroyAllocator(m_memAllocator);

        // Destroy device handle.
        if (m_handle)
            vkDestroyDevice(m_handle, nullptr);
    }

    VkResult Device::UncompressedImageSubData(Image* pImage, const VezImageSubDataInfo* pSubDataInfo, const void* pData)
    {
        // Get a one-time submit command buffer.
        auto cmdBuffer = GetOneTimeSubmitCommandBuffer();

        // Get the queue family index the command buffer was created for.
        auto queue = m_queues[cmdBuffer->GetPool()->GetQueueFamilyIndex()][0];
        
        // Get required alignment flush size for selected physical device.
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(m_physicalDevice->GetHandle(), &properties);
        auto alignedFlushSize = properties.limits.nonCoherentAtomSize;

        // Determine pixel size of source buffer.
        auto pixelSize = GetUncompressedImageFormatSize(pImage->GetCreateInfo().format);

        // Determine total number of pixel blocks that can be transfered at once.
        auto pinnedMemoryBufferSize = m_pinnedMemoryBuffer->GetCreateInfo().size;
        auto numSlices = std::min(pSubDataInfo->imageExtent.depth, static_cast<uint32_t>(pinnedMemoryBufferSize / (pixelSize * pSubDataInfo->imageExtent.width * pSubDataInfo->imageExtent.height)));
        auto numRows = std::min(pSubDataInfo->imageExtent.height, static_cast<uint32_t>(pinnedMemoryBufferSize / (pixelSize * pSubDataInfo->imageExtent.width)));
        auto numCols = std::min(pSubDataInfo->imageExtent.width, static_cast<uint32_t>(pinnedMemoryBufferSize / pixelSize));
        auto maxExtent = VkExtent3D{ numCols, std::max(1U, numRows), std::max(1U, numSlices) };

        // Get 8-bit starting address of pixel data.
        const uint8_t* srcPtr = reinterpret_cast<const uint8_t*>(pData);

        // Moving offset and extents during copy.
        auto curOffset = pSubDataInfo->imageOffset;

        // Each layer's data is copied separately.
        auto layerCount = pSubDataInfo->imageSubresource.layerCount;
        if (layerCount == VK_REMAINING_ARRAY_LAYERS)
            layerCount = pImage->GetCreateInfo().arrayLayers - pSubDataInfo->imageSubresource.baseArrayLayer;

        for (auto arrayLayer = pSubDataInfo->imageSubresource.baseArrayLayer; arrayLayer < pSubDataInfo->imageSubresource.baseArrayLayer + layerCount; ++arrayLayer)
        {
            // Each depth layer in a 3D texture is copied separately.
            for (auto z = 0U; z < pSubDataInfo->imageExtent.depth; z += maxExtent.depth)
            {
                // Set current destination image z offset.
                curOffset.z = z;

                // Handle end of image depth case.
                auto slicesToCopy = std::min(maxExtent.depth, pSubDataInfo->imageExtent.depth - z);

                // Copy pixel data with scrolling extents window moving across 2D image layer.
                for (auto row = 0U; row < pSubDataInfo->imageExtent.height; row += maxExtent.height)
                {
                    // Handle end of image height case.
                    auto rowsToCopy = std::min(maxExtent.height, pSubDataInfo->imageExtent.height - row);

                    for (auto col = 0U; col < pSubDataInfo->imageExtent.width; col += maxExtent.width)
                    {
                        // Handle end of image width case.
                        auto colsToCopy = std::min(maxExtent.width, pSubDataInfo->imageExtent.width - col);
                        auto bytesCopied = 0;

                        // Copy the host memory to the pinned memory buffer.
                        if (pSubDataInfo->dataRowLength == 0)
                        {
                            auto bytesToCopy = colsToCopy * rowsToCopy * slicesToCopy * pixelSize;
                            memcpy(m_pinnedMemoryPtr, srcPtr, bytesToCopy);
                            srcPtr += bytesToCopy;
                            bytesCopied = bytesToCopy;
                        }
                        else
                        {
                            for (auto y = 0U; y < rowsToCopy; ++y)
                            {
                                memcpy(&reinterpret_cast<uint8_t*>(m_pinnedMemoryPtr)[y * maxExtent.width * pixelSize], srcPtr, colsToCopy * pixelSize);
                                srcPtr += pSubDataInfo->dataRowLength * pixelSize;
                                bytesCopied = colsToCopy * pixelSize;
                            }
                        }

                        // Flush must be aligned according to physical device's limits.
                        auto alignedBytesToCopy = static_cast<VkDeviceSize>(std::ceil(bytesCopied / static_cast<float>(alignedFlushSize))) * alignedFlushSize;

                        // Flush the memory write.
                        VmaAllocationInfo allocInfo = {};
                        vmaGetAllocationInfo(m_memAllocator, m_pinnedMemoryBuffer->GetAllocation(), &allocInfo);

                        VkMappedMemoryRange memoryRange = {};
                        memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                        memoryRange.memory = allocInfo.deviceMemory;
                        memoryRange.offset = 0;
                        memoryRange.size = alignedBytesToCopy;
                        vkFlushMappedMemoryRanges(m_handle, 1, &memoryRange);

                        // Copy the pinned memory buffer to the destination image.
                        VezBufferImageCopy region = {};
                        //region.bufferRowLength = std::max(0U, std::min(pSubDataInfo->dataRowLength, pSubDataInfo->dataRowLength - col * pixelSize));
                        //region.bufferImageHeight = std::max(0U, std::min(pSubDataInfo->dataImageHeight, pSubDataInfo->dataImageHeight - row));
                        region.imageSubresource.baseArrayLayer = arrayLayer;
                        region.imageSubresource.mipLevel = pSubDataInfo->imageSubresource.mipLevel;
                        region.imageSubresource.layerCount = 1;
                        region.imageOffset = curOffset;
                        region.imageExtent = VkExtent3D{ colsToCopy, rowsToCopy, slicesToCopy };
                        cmdBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
                        cmdBuffer->CmdCopyBufferToImage(m_pinnedMemoryBuffer, pImage, 1, &region);
                        cmdBuffer->End();

                        // Submit to the queue.
                        auto cmdBufferHandle = cmdBuffer->GetHandle();

                        VezSubmitInfo submitInfo = {};
                        submitInfo.commandBufferCount = 1;
                        submitInfo.pCommandBuffers = &cmdBufferHandle;
                        auto result = queue->Submit(1, &submitInfo, nullptr);
                        if (result != VK_SUCCESS)
                            return result;

                        // Wait for all operations to complete.
                        queue->WaitIdle();

                        // Update current destination image x offset.
                        curOffset.x += colsToCopy;
                        if (curOffset.x >= static_cast<int32_t>(pSubDataInfo->imageExtent.width))
                            curOffset.x = static_cast<int32_t>(pSubDataInfo->imageOffset.x);
                    }

                    // Update current destination image y offset.
                    curOffset.y += rowsToCopy;
                    if (curOffset.y >= static_cast<int32_t>(pSubDataInfo->imageExtent.height))
                        curOffset.y = static_cast<int32_t>(pSubDataInfo->imageOffset.y);
                }

                // Update destination image z offset.
                curOffset.z += slicesToCopy;
                if (curOffset.z >= static_cast<int32_t>(pSubDataInfo->imageExtent.depth))
                    curOffset.z = static_cast<int32_t>(pSubDataInfo->imageOffset.z);
            }

            // Increment source pointer address.
            if (pSubDataInfo->dataImageHeight != 0)
            {
                if (pSubDataInfo->dataRowLength != 0) srcPtr += pSubDataInfo->dataRowLength * pixelSize * (pSubDataInfo->dataImageHeight - pSubDataInfo->imageExtent.height);
                else srcPtr += pSubDataInfo->imageExtent.width * pixelSize * (pSubDataInfo->dataImageHeight - pSubDataInfo->imageExtent.height);
            }
        }

        // Return success.
        return VK_SUCCESS;
    }

    VkResult Device::CompressedImageSubData(Image* pImage, const VezImageSubDataInfo* pSubDataInfo, const void* pData)
    {
        // Get a one-time submit command buffer.
        auto cmdBuffer = GetOneTimeSubmitCommandBuffer();

        // Get the queue family index the command buffer was created for.
        auto queue = m_queues[cmdBuffer->GetPool()->GetQueueFamilyIndex()][0];

        // Get required alignment flush size for selected physical device.
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(m_physicalDevice->GetHandle(), &properties);
        auto alignedFlushSize = properties.limits.nonCoherentAtomSize;

        // Determine the size of each compressed block.
        uint32_t blockSize = 0U, blockWidth = 0U, blockHeight = 0U;
        GetCompressedImageFormatInfo(pImage->GetCreateInfo().format, blockSize, blockWidth, blockHeight);

        // Compute number of compressed blocks making up image data.
        uint32_t numBlocksX = pSubDataInfo->imageExtent.width / blockWidth;
        uint32_t numBlocksY = pSubDataInfo->imageExtent.height / blockHeight;

        // Determine total number of compressed blocks that can be transferred at once.
        auto pinnedMemoryBufferSize = m_pinnedMemoryBuffer->GetCreateInfo().size;
        auto maxTransferBlocksY = std::min(numBlocksY, static_cast<uint32_t>(pinnedMemoryBufferSize / (blockSize * numBlocksX)));
        auto maxTransferBlocksX = std::min(numBlocksX, static_cast<uint32_t>(pinnedMemoryBufferSize / blockSize));
        auto maxExtent = VkExtent2D{ maxTransferBlocksX, std::max(1U, maxTransferBlocksX) };

        // Get 8-bit starting address of pixel data.
        const uint8_t* srcPtr = reinterpret_cast<const uint8_t*>(pData);

        // Moving offset and extents during copy.
        auto curOffset = pSubDataInfo->imageOffset;

        // Each layer's data is copied separately.
        auto layerCount = pSubDataInfo->imageSubresource.layerCount;
        if (layerCount == VK_REMAINING_ARRAY_LAYERS)
            layerCount = pImage->GetCreateInfo().arrayLayers - pSubDataInfo->imageSubresource.baseArrayLayer;

        for (auto layer = pSubDataInfo->imageSubresource.baseArrayLayer; layer <  pSubDataInfo->imageSubresource.baseArrayLayer + layerCount; ++layer)
        {
            // Each depth layer in a 3D texture is copied separately.
            for (auto z = 0U; z < pSubDataInfo->imageExtent.depth; ++z)
            {
                // Set current destination image z offset.
                curOffset.z = z;

                // Copy compressed block data with scrolling extents window moving across 2D image layer.
                for (auto row = 0U; row < numBlocksY; row += maxExtent.height)
                {
                    // Handle end of image height case.
                    auto rowsToCopy = std::min(maxExtent.height, numBlocksY - row);

                    for (auto col = 0U; col < numBlocksX; col += maxExtent.width)
                    {
                        // Handle end of image width case.
                        auto colsToCopy = std::min(maxExtent.width, numBlocksX - col);
                        auto bytesCopied = 0;

                        // Copy the host memory to the pinned memory buffer.
                        if (pSubDataInfo->dataRowLength == 0)
                        {
                            auto bytesToCopy = colsToCopy * rowsToCopy * blockSize;
                            memcpy(m_pinnedMemoryPtr, srcPtr, bytesToCopy);
                            srcPtr += bytesToCopy;
                            bytesCopied = bytesToCopy;
                        }
                        else
                        {
                            for (auto y = 0U; y < rowsToCopy; ++y)
                            {
                                memcpy(&reinterpret_cast<uint8_t*>(m_pinnedMemoryPtr)[y * maxExtent.width * blockSize], srcPtr, colsToCopy * blockSize);
                                srcPtr += pSubDataInfo->dataRowLength * blockSize;
                                bytesCopied = colsToCopy * blockSize;
                            }
                        }

                        // Flush must be aligned according to physical device's limits.
                        auto alignedBytesToCopy = static_cast<VkDeviceSize>(std::ceil(bytesCopied / static_cast<float>(alignedFlushSize))) * alignedFlushSize;

                        // Flush the memory write.
                        VmaAllocationInfo allocInfo = {};
                        vmaGetAllocationInfo(m_memAllocator, m_pinnedMemoryBuffer->GetAllocation(), &allocInfo);

                        VkMappedMemoryRange memoryRange = {};
                        memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                        memoryRange.memory = allocInfo.deviceMemory;
                        memoryRange.offset = 0;
                        memoryRange.size = alignedBytesToCopy;
                        vkFlushMappedMemoryRanges(m_handle, 1, &memoryRange);

                        // Copy the pinned memory buffer to the destination image.
                        VezBufferImageCopy region = {};
                        //region.bufferRowLength = std::max(0U, std::min(pSubDataInfo->dataRowLength, pSubDataInfo->dataRowLength - col * pixelSize));
                        //region.bufferImageHeight = std::max(0U, std::min(pSubDataInfo->dataImageHeight, pSubDataInfo->dataImageHeight - row));
                        region.imageSubresource.baseArrayLayer = layer;
                        region.imageSubresource.mipLevel = pSubDataInfo->imageSubresource.mipLevel;
                        region.imageSubresource.layerCount = 1;
                        region.imageOffset = curOffset;
                        region.imageExtent = VkExtent3D{ colsToCopy * blockWidth, rowsToCopy * blockHeight, 1U };
                        cmdBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
                        cmdBuffer->CmdCopyBufferToImage(m_pinnedMemoryBuffer, pImage, 1, &region);
                        cmdBuffer->End();

                        // Submit to the queue.
                        auto cmdBufferHandle = cmdBuffer->GetHandle();
                        VezSubmitInfo submitInfo = {};
                        submitInfo.commandBufferCount = 1;
                        submitInfo.pCommandBuffers = &cmdBufferHandle;
                        auto result = queue->Submit(1, &submitInfo, nullptr);
                        if (result != VK_SUCCESS)
                            return result;

                        // Wait for all operations to complete.
                        queue->WaitIdle();

                        // Update current destination image x offset.
                        curOffset.x += colsToCopy * blockWidth;
                        if (curOffset.x >= static_cast<int32_t>(pSubDataInfo->imageExtent.width))
                            curOffset.x = static_cast<int32_t>(pSubDataInfo->imageOffset.x);
                    }

                    // Update current destination image y offset.
                    curOffset.y += rowsToCopy * blockHeight;
                    if (curOffset.y >= static_cast<int32_t>(pSubDataInfo->imageExtent.height))
                        curOffset.y = static_cast<int32_t>(pSubDataInfo->imageOffset.y);
                }
            }

            // Increment source pointer address.
            if (pSubDataInfo->dataImageHeight != 0)
            {
                if (pSubDataInfo->dataRowLength != 0) srcPtr += pSubDataInfo->dataRowLength * blockSize * ((pSubDataInfo->dataImageHeight - pSubDataInfo->imageExtent.height) / blockHeight);
                else srcPtr += (pSubDataInfo->imageExtent.width / blockWidth) * blockSize * ((pSubDataInfo->dataImageHeight - pSubDataInfo->imageExtent.height) / blockHeight);
            }
        }

        // Return success.
        return VK_SUCCESS;
    }

    CommandBuffer* Device::GetOneTimeSubmitCommandBuffer()
    {
        CommandBuffer* commandBuffer = nullptr;

        // Make thread-safe.
        BEGIN_THREAD_SAFE_BLOCK();

        // Find the one-time submit command buffer for the current thread.
        auto itr = m_oneTimeSubmitCommandBuffers.find(std::this_thread::get_id());
        if (itr != m_oneTimeSubmitCommandBuffers.end())
        {
            commandBuffer = itr->second;
        }
        else
        {
            // Create a new CommandBuffer instance.
            // By default one-time submit command buffers use the first queue family.
            if (AllocateCommandBuffers(m_queues[0][0], nullptr, 1, &commandBuffer, KILOBYTES(8)) == VK_SUCCESS)
                m_oneTimeSubmitCommandBuffers.emplace(std::this_thread::get_id(), commandBuffer);
        }

        // End thread-safe block.
        END_THREAD_SAFE_BLOCK();

        // Return result.
        return commandBuffer;
    }    
}