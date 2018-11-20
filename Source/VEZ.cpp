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
#include <cstring>
#include <vector>
#include "VEZ.h"
#include "Utility/ObjectLookup.h"
#include "Core/Instance.h"
#include "Core/PhysicalDevice.h"
#include "Core/Device.h"
#include "Core/Queue.h"
#include "Core/Swapchain.h"
#include "Core/SyncPrimitivesPool.h"
#include "Core/CommandPool.h"
#include "Core/CommandBuffer.h"
#include "Core/ShaderModule.h"
#include "Core/Pipeline.h"
#include "Core/VertexInputFormat.h"
#include "Core/Buffer.h"
#include "Core/BufferView.h"
#include "Core/Image.h"
#include "Core/ImageView.h"
#include "Core/Framebuffer.h"

// Per thread command buffer currently being recorded.
static thread_local vez::CommandBuffer* s_pActiveCommandBuffer = nullptr;

// Utility function for importing an external VkBuffer object handle into VEZ.
vez::Buffer* ImportVkBuffer(vez::Device* device, VkBuffer buffer)
{
    VezBufferCreateInfo createInfo = {};
    auto bufferImpl = vez::Buffer::CreateFromHandle(device, &createInfo, buffer, VK_NULL_HANDLE);
    vez::ObjectLookup::AddObjectImpl(buffer, bufferImpl);
    return bufferImpl;
}

VkResult VKAPI_CALL vezEnumerateInstanceExtensionProperties(const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties)
{
    return vkEnumerateInstanceExtensionProperties(pLayerName, pPropertyCount, pProperties);
}

VkResult VKAPI_CALL vezEnumerateInstanceLayerProperties(uint32_t* pPropertyCount, VkLayerProperties*pProperties)
{
    return vkEnumerateInstanceLayerProperties(pPropertyCount, pProperties);
}

VkResult VKAPI_CALL vezCreateInstance(const VezInstanceCreateInfo* pCreateInfo, VkInstance* pInstance)
{
    // Create a new Instance object.
    vez::Instance* instanceImpl = nullptr;
    auto result = vez::Instance::Create(pCreateInfo, &instanceImpl);
    if (result != VK_SUCCESS)
        return result;

    // Store native Vulkan handle in pInstance parameter.
    *pInstance = instanceImpl->GetHandle();

    // Add to ObjectLookup.
    vez::ObjectLookup::AddObjectImpl(*pInstance, instanceImpl);

    // Add all of the instance's physical devices to ObjectLookup.
    const auto& physicalDevices = instanceImpl->GetPhysicalDevices();
    for (auto pd : physicalDevices)
        vez::ObjectLookup::AddObjectImpl(pd->GetHandle(), pd);

    // Return success.
    return VK_SUCCESS;
}

void VKAPI_CALL vezDestroyInstance(VkInstance instance)
{
    // Lookup object handle.
    auto instanceImpl = vez::ObjectLookup::GetImplInstance(instance);
    if (instanceImpl)
    {
        // Remove instance's physical devices from ObjectLookup.
        const auto& physicalDevices = instanceImpl->GetPhysicalDevices();
        for (auto pd : physicalDevices)
            vez::ObjectLookup::RemoveImplPhysicalDevice(pd->GetHandle());

        // Destroy Instance object and remove from ObjectLookup.
        vez::Instance::Destroy(instanceImpl);
        vez::ObjectLookup::RemoveImplInstance(instance);
    }
}

VkResult VKAPI_CALL vezEnumeratePhysicalDevices(VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices)
{
    // Lookup Instance object handle.
    auto instanceImpl = vez::ObjectLookup::GetImplInstance(instance);
    if (!instanceImpl)
        return VK_INCOMPLETE;

    // Retrieve the list of physical devices associated with the instance.
    const auto& physicalDevices = instanceImpl->GetPhysicalDevices();
    if (pPhysicalDeviceCount)
    {
        *pPhysicalDeviceCount = static_cast<uint32_t>(physicalDevices.size());
        if (pPhysicalDevices)
        {
            for (auto i = 0U; i < *pPhysicalDeviceCount; ++i)
            {
                pPhysicalDevices[i] = physicalDevices[i]->GetHandle();
            }
        }
    }

    // Return success.
    return VK_SUCCESS;
}

void VKAPI_CALL vezGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties)
{
    vkGetPhysicalDeviceProperties(physicalDevice, pProperties);
}

void VKAPI_CALL vezGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* pFeatures)
{
    vkGetPhysicalDeviceFeatures(physicalDevice, pFeatures);
}

void VKAPI_CALL vezGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties* pFormatProperties)
{
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, pFormatProperties);
}

VkResult VKAPI_CALL vezGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties)
{
    return vkGetPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);
}

void VKAPI_CALL vezGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties* pQueueFamilyProperties)
{
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

VkResult VKAPI_CALL vezGetPhysicalDeviceSurfaceFormats(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats)
{
    return vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, pSurfaceFormatCount, pSurfaceFormats);
}

VkResult VKAPI_CALL vezGetPhysicalDevicePresentSupport(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32* pSupported)
{
    return vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, pSupported);
}

VkResult VKAPI_CALL vezEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties)
{
    return vkEnumerateDeviceExtensionProperties(physicalDevice, pLayerName, pPropertyCount, pProperties);
}

VkResult VKAPI_CALL vezEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount, VkLayerProperties* pProperties)
{
    return vkEnumerateDeviceLayerProperties(physicalDevice, pPropertyCount, pProperties);
}

VkResult VKAPI_CALL vezCreateDevice(VkPhysicalDevice physicalDevice, const VezDeviceCreateInfo* pCreateInfo, VkDevice* pDevice)
{
    // Lookup PhysicalDevice object handle.
    auto physicalDeviceImpl = vez::ObjectLookup::GetImplPhysicalDevice(physicalDevice);
    if (!physicalDeviceImpl)
        return VK_INCOMPLETE;

    // Create the device.
    vez::Device* deviceImpl = nullptr;
    auto result = vez::Device::Create(physicalDeviceImpl, pCreateInfo, &deviceImpl);
    if (result != VK_SUCCESS)
        return result;

    // Store native Vulkan handle in pInstance parameter.
    *pDevice = deviceImpl->GetHandle();

    // Add to ObjectLookup.
    vez::ObjectLookup::AddObjectImpl(*pDevice, deviceImpl);

    // Add all of the devices's queues to ObjectLookup.
    const auto& queueFamilies = deviceImpl->GetQueueFamilies();
    for (auto family : queueFamilies)
        for (auto queue : family)
            vez::ObjectLookup::AddObjectImpl(queue->GetHandle(), queue);

    // Return success.
    return VK_SUCCESS;
}

void VKAPI_CALL vezDestroyDevice(VkDevice device)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (deviceImpl)
    {
        // Remove devices's queues from ObjectLookup.
        const auto& queueFamilies = deviceImpl->GetQueueFamilies();
        for (auto family : queueFamilies)
            for (auto queue : family)
                vez::ObjectLookup::RemoveImplQueue(reinterpret_cast<VkQueue>(queue->GetHandle()));

        // Destroy Device object and remove from ObjectLookup.
        vez::Device::Destroy(deviceImpl);
        vez::ObjectLookup::RemoveImplDevice(device);
    }
}

VkResult VKAPI_CALL vezDeviceWaitIdle(VkDevice device)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (!deviceImpl)
        return VK_INCOMPLETE;

    // Call class method.
    return deviceImpl->WaitIdle();
}

void VKAPI_CALL vezGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (deviceImpl)
    {
        // Get the queue for the specified family and index.
        auto queueImpl = deviceImpl->GetQueue(queueFamilyIndex, queueIndex);

        // Copy object handle.
        *pQueue = reinterpret_cast<VkQueue>(queueImpl->GetHandle());
    }
}

void VKAPI_CALL vezGetDeviceGraphicsQueue(VkDevice device, uint32_t queueIndex, VkQueue* pQueue)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (deviceImpl)
    {
        // Get the queue for the specified family and index.
        auto queueImpl = deviceImpl->GetQueueByFlags(VK_QUEUE_GRAPHICS_BIT, queueIndex);

        // Copy object handle.
        *pQueue = reinterpret_cast<VkQueue>(queueImpl->GetHandle());
    }
}

void VKAPI_CALL vezGetDeviceComputeQueue(VkDevice device, uint32_t queueIndex, VkQueue* pQueue)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (deviceImpl)
    {
        // Get the queue for the specified family and index.
        auto queueImpl = deviceImpl->GetQueueByFlags(VK_QUEUE_COMPUTE_BIT, queueIndex);

        // Copy object handle.
        *pQueue = reinterpret_cast<VkQueue>(queueImpl->GetHandle());
    }
}

void VKAPI_CALL vezGetDeviceTransferQueue(VkDevice device, uint32_t queueIndex, VkQueue* pQueue)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (deviceImpl)
    {
        // Get the queue for the specified family and index.
        auto queueImpl = deviceImpl->GetQueueByFlags(VK_QUEUE_TRANSFER_BIT, queueIndex);

        // Copy object handle.
        *pQueue = reinterpret_cast<VkQueue>(queueImpl->GetHandle());
    }
}

VkResult VKAPI_CALL vezCreateSwapchain(VkDevice device, const VezSwapchainCreateInfo* pCreateInfo, VezSwapchain* pSwapchain)
{
    // Lookup Device object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (!device)
        return VK_INCOMPLETE;

    // Create the swapchain.
    vez::Swapchain* swapchainImpl = nullptr;
    auto result = vez::Swapchain::Create(deviceImpl, pCreateInfo, &swapchainImpl);
    if (result != VK_SUCCESS)
        return result;

    // Store native class object handle in pSwapchain parameter.
    *pSwapchain = reinterpret_cast<VezSwapchain>(swapchainImpl);

    // Return success.
    return VK_SUCCESS;
}

void VKAPI_CALL vezDestroySwapchain(VkDevice device, VezSwapchain swapchain)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (deviceImpl)
    {
        // Destroy swapchain.
        delete reinterpret_cast<vez::Swapchain*>(swapchain);
    }
}

void VKAPI_CALL vezGetSwapchainSurfaceFormat(VezSwapchain swapchain, VkSurfaceFormatKHR* pFormat)
{
    auto surfaceFormat = reinterpret_cast<vez::Swapchain*>(swapchain)->GetFormat();
    memcpy(pFormat, &surfaceFormat, sizeof(VkSurfaceFormatKHR));
}

VkResult VKAPI_CALL vezSwapchainSetVSync(VezSwapchain swapchain, VkBool32 enabled)
{
    return reinterpret_cast<vez::Swapchain*>(swapchain)->SetVSync(enabled);
}

VkResult VKAPI_CALL vezQueueSubmit(VkQueue queue, uint32_t submitCount, const VezSubmitInfo* pSubmits, VkFence* pFence)
{
    // Lookup Queue object handle.
    auto queueImpl = vez::ObjectLookup::GetImplQueue(queue);
    if (!queueImpl)
        return VK_INCOMPLETE;

    // Forward the submit onto the queue.
    return queueImpl->Submit(submitCount, pSubmits, pFence);
}

VkResult VKAPI_CALL vezQueuePresent(VkQueue queue, const VezPresentInfo* pPresentInfo)
{
    // Lookup Queue object handle.
    auto queueImpl = vez::ObjectLookup::GetImplQueue(queue);
    if (!queueImpl)
        return VK_INCOMPLETE;

    // Forward the submit onto the queue's device.
    return queueImpl->Present(pPresentInfo);
}

VkResult VKAPI_CALL vezQueueWaitIdle(VkQueue queue)
{
    // Lookup Queue object handle.
    auto queueImpl = vez::ObjectLookup::GetImplQueue(queue);
    if (!queueImpl)
        return VK_INCOMPLETE;

    return queueImpl->WaitIdle();
}

void VKAPI_CALL vezDestroyFence(VkDevice device, VkFence fence)
{
    // Lookup device object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (deviceImpl)
    {
        // Lookup fence object handle.
        auto fenceImpl = vez::ObjectLookup::GetImplFence(fence);
        if (fenceImpl)
            deviceImpl->DestroyFence(fenceImpl);
    }
}

VkResult VKAPI_CALL vezGetFenceStatus(VkDevice device, VkFence fence)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (deviceImpl) return static_cast<VkResult>(vkGetFenceStatus(deviceImpl->GetHandle(), reinterpret_cast<VkFence>(fence)));
    else return VK_INCOMPLETE;
}

VkResult VKAPI_CALL vezWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (deviceImpl) return vkWaitForFences(deviceImpl->GetHandle(), fenceCount, reinterpret_cast<const VkFence*>(pFences), waitAll, timeout);
    else return VK_INCOMPLETE;
}

void VKAPI_CALL vezDestroySemaphore(VkDevice device, VkSemaphore semaphore)
{
    // Lookup device object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (deviceImpl)
        deviceImpl->DestroySemaphore(semaphore);
}

VkResult VKAPI_CALL vezCreateEvent(VkDevice device, VkEvent* pEvent)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (!deviceImpl)
        return VK_INCOMPLETE;

    // Create the event.
    VkEventCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    return vkCreateEvent(deviceImpl->GetHandle(), &createInfo, nullptr, pEvent);
}

void VKAPI_CALL vezDestroyEvent(VkDevice device, VkEvent event)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (deviceImpl)
        vkDestroyEvent(deviceImpl->GetHandle(), event, nullptr);
}

VkResult VKAPI_CALL vezGetEventStatus(VkDevice device, VkEvent event)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (deviceImpl) return vkGetEventStatus(deviceImpl->GetHandle(), event);
    else return VK_INCOMPLETE;
}

VkResult VKAPI_CALL vezSetEvent(VkDevice device, VkEvent event)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (deviceImpl) return vkSetEvent(deviceImpl->GetHandle(), event);
    else return VK_INCOMPLETE;
}

VkResult VKAPI_CALL vezResetEvent(VkDevice device, VkEvent event)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (deviceImpl) vkResetEvent(deviceImpl->GetHandle(), event);
    return VK_INCOMPLETE;
}

VkResult VKAPI_CALL vezCreateQueryPool(VkDevice device, const VezQueryPoolCreateInfo* pCreateInfo, VkQueryPool* pQueryPool)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (!deviceImpl)
        return VK_INCOMPLETE;

    // Create the query pool.
    VkQueryPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    createInfo.pNext = pCreateInfo->pNext;
    createInfo.queryType = pCreateInfo->queryType;
    createInfo.queryCount = pCreateInfo->queryCount;
    createInfo.pipelineStatistics = pCreateInfo->pipelineStatistics;
    return vkCreateQueryPool(deviceImpl->GetHandle(), &createInfo, nullptr, reinterpret_cast<VkQueryPool*>(pQueryPool));
}

void VKAPI_CALL vezDestroyQueryPool(VkDevice device, VkQueryPool queryPool)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (deviceImpl)
        vkDestroyQueryPool(deviceImpl->GetHandle(), queryPool, nullptr);
}

VkResult VKAPI_CALL vezGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (deviceImpl) return vkGetQueryPoolResults(deviceImpl->GetHandle(), queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
    else return VK_INCOMPLETE;

}

VkResult VKAPI_CALL vezCreateShaderModule(VkDevice device, const VezShaderModuleCreateInfo* pCreateInfo, VkShaderModule* pShaderModule)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (!deviceImpl)
        return VK_INCOMPLETE;

    // Create the shader module.
    vez::ShaderModule* shaderModule = nullptr;
    auto result = vez::ShaderModule::Create(deviceImpl, pCreateInfo, &shaderModule);

    // If shaderModule was successfully created, store it regardless of the return code since the GLSL compilation log must be retrievable.
    if (shaderModule)
    {
        // Store native Vulkan handle in pShaderModule parameter.
        *pShaderModule = shaderModule->GetHandle();

        // Add to ObjectLookup.
        vez::ObjectLookup::AddObjectImpl(*pShaderModule, shaderModule);
    }

    // Return the result.
    return result;
}

void VKAPI_CALL vezDestroyShaderModule(VkDevice device, VkShaderModule shaderModule)
{
    // Lookup object handle.
    auto shaderModuleImpl = vez::ObjectLookup::GetImplShaderModule(shaderModule);
    if (shaderModuleImpl)
    {
        vez::ObjectLookup::RemoveImplShaderModule(shaderModule);
        delete shaderModuleImpl;
    }
}

void VKAPI_CALL vezGetShaderModuleInfoLog(VkShaderModule shaderModule, uint32_t* pLength, char* pInfoLog)
{
    // Lookup object handle.
    auto shaderModuleImpl = vez::ObjectLookup::GetImplShaderModule(shaderModule);
    if (shaderModuleImpl)
    {
        const std::string& infoLog = shaderModuleImpl->GetInfoLog();
        *pLength = static_cast<uint32_t>(infoLog.size());
        if (pInfoLog)
            memcpy(pInfoLog, infoLog.c_str(), infoLog.size());
    }
}

VkResult VKAPI_CALL vezGetShaderModuleBinary(VkShaderModule shaderModule, uint32_t* pLength, uint32_t* pBinary)
{
    // Lookup object handle.
    auto shaderModuleImpl = vez::ObjectLookup::GetImplShaderModule(shaderModule);
    if (!shaderModuleImpl)
        return VK_INCOMPLETE;

    return shaderModuleImpl->GetBinary(pLength, pBinary);
}

VkResult VKAPI_CALL vezCreateGraphicsPipeline(VkDevice device, const VezGraphicsPipelineCreateInfo* pCreateInfo, VezPipeline* pPipeline)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (!deviceImpl)
        return VK_INCOMPLETE;

    // Create the graphics pipeline.
    return vez::Pipeline::Create(deviceImpl, pCreateInfo, reinterpret_cast<vez::Pipeline**>(pPipeline));
}

VkResult VKAPI_CALL vezCreateComputePipeline(VkDevice device, const VezComputePipelineCreateInfo* pCreateInfo, VezPipeline* pPipeline)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (!deviceImpl)
        return VK_INCOMPLETE;

    // Create the compute pipeline.
    return vez::Pipeline::Create(deviceImpl, pCreateInfo, reinterpret_cast<vez::Pipeline**>(pPipeline));
}

void VKAPI_CALL vezDestroyPipeline(VkDevice device, VezPipeline pipeline)
{
    delete reinterpret_cast<vez::Pipeline*>(pipeline);
}

VkResult VKAPI_CALL vezEnumeratePipelineResources(VezPipeline pipeline, uint32_t* pResourceCount, VezPipelineResource* pResources)
{
    return reinterpret_cast<vez::Pipeline*>(pipeline)->EnumeratePipelineResources(pResourceCount, pResources);
}

VkResult VKAPI_CALL vezGetPipelineResource(VezPipeline pipeline, const char* name, VezPipelineResource* pResource)
{
    return reinterpret_cast<vez::Pipeline*>(pipeline)->GetPipelineResource(name, pResource);
}

VkResult VKAPI_CALL vezCreateVertexInputFormat(VkDevice device, const VezVertexInputFormatCreateInfo* pCreateInfo, VezVertexInputFormat* pFormat)
{
    return vez::VertexInputFormat::Create(pCreateInfo, reinterpret_cast<vez::VertexInputFormat**>(pFormat));
}

void VKAPI_CALL vezDestroyVertexInputFormat(VkDevice device, VezVertexInputFormat format)
{
    delete reinterpret_cast<vez::VertexInputFormat*>(format);
}

VkResult VKAPI_CALL vezCreateSampler(VkDevice device, const VezSamplerCreateInfo* pCreateInfo, VkSampler* pSampler)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (!deviceImpl)
        return VK_INCOMPLETE;

    // Create the sampler.
    VkSamplerCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = pCreateInfo->magFilter;
    createInfo.minFilter = pCreateInfo->minFilter;
    createInfo.mipmapMode = pCreateInfo->mipmapMode;
    createInfo.addressModeU = pCreateInfo->addressModeU;
    createInfo.addressModeV = pCreateInfo->addressModeV;
    createInfo.addressModeW = pCreateInfo->addressModeW;
    createInfo.mipLodBias = pCreateInfo->mipLodBias;
    createInfo.anisotropyEnable = pCreateInfo->anisotropyEnable;
    createInfo.maxAnisotropy = pCreateInfo->maxAnisotropy;
    createInfo.compareEnable = pCreateInfo->compareEnable;
    createInfo.compareOp = pCreateInfo->compareOp;
    createInfo.minLod = pCreateInfo->minLod;
    createInfo.maxLod = pCreateInfo->maxLod;
    createInfo.borderColor = pCreateInfo->borderColor;
    createInfo.unnormalizedCoordinates = pCreateInfo->unnormalizedCoordinates;
    return vkCreateSampler(deviceImpl->GetHandle(), &createInfo, nullptr, pSampler);
}

void VKAPI_CALL vezDestroySampler(VkDevice device, VkSampler sampler)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (deviceImpl)
        vkDestroySampler(deviceImpl->GetHandle(), sampler, nullptr);
}

VkResult VKAPI_CALL vezCreateBuffer(VkDevice device, VezMemoryFlags memFlags, const VezBufferCreateInfo* pCreateInfo, VkBuffer* pBuffer)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (!deviceImpl)
        return VK_INCOMPLETE;

    // Create the buffer.
    vez::Buffer* bufferImpl = nullptr;
    auto result = deviceImpl->CreateBuffer(memFlags, pCreateInfo, &bufferImpl);
    if (result != VK_SUCCESS)
        return result;

    // Store native Vulkan handle in pBuffer parameter.
    *pBuffer = reinterpret_cast<VkBuffer>(bufferImpl->GetHandle());

    // Add to ObjectLookup.
    vez::ObjectLookup::AddObjectImpl(*pBuffer, bufferImpl);

    // Return success.
    return VK_SUCCESS;
}

void VKAPI_CALL vezDestroyBuffer(VkDevice device, VkBuffer buffer)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (deviceImpl)
    {
        // Lookup Buffer object handle.
        auto bufferImpl = vez::ObjectLookup::GetImplBuffer(buffer);
        if (bufferImpl)
        {
            // Remove the Buffer object from ObjectLookup and destroy the instance.
            vez::ObjectLookup::RemoveImplBuffer(buffer);
            deviceImpl->DestroyBuffer(bufferImpl);
        }
    }
}

VkResult VKAPI_CALL vezBufferSubData(VkDevice device, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, const void* pData)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (!deviceImpl)
        return VK_INCOMPLETE;

    // Lookup Buffer object handle.
    auto bufferImpl = vez::ObjectLookup::GetImplBuffer(buffer);
    if (!bufferImpl)
        return VK_INCOMPLETE;

    // Upload data to buffer.
    return deviceImpl->BufferSubData(bufferImpl, offset, size, pData);
}

VkResult VKAPI_CALL vezMapBuffer(VkDevice device, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, void** ppData)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (!deviceImpl)
        return VK_INCOMPLETE;

    // Lookup Buffer object handle.
    auto bufferImpl = vez::ObjectLookup::GetImplBuffer(buffer);
    if (!bufferImpl)
        return VK_INCOMPLETE;

    // Map buffer.
    return deviceImpl->MapBuffer(bufferImpl, offset, size, ppData);
}

void VKAPI_CALL vezUnmapBuffer(VkDevice device, VkBuffer buffer)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (!deviceImpl)
        return;

    // Lookup Buffer object handle.
    auto bufferImpl = vez::ObjectLookup::GetImplBuffer(buffer);
    if (!bufferImpl)
        return;

    // Unmap buffer.
    deviceImpl->UnmapBuffer(bufferImpl);
}

VkResult VKAPI_CALL vezFlushMappedBufferRanges(VkDevice device, uint32_t bufferRangeCount, const VezMappedBufferRange* pBufferRanges)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (deviceImpl) return deviceImpl->FlushMappedBufferRanges(bufferRangeCount, pBufferRanges);
    else return VK_INCOMPLETE;
}

VkResult VKAPI_CALL vezInvalidateMappedBufferRanges(VkDevice device, uint32_t bufferRangeCount, const VezMappedBufferRange* pBufferRanges)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (deviceImpl) return deviceImpl->InvalidateMappedBufferRanges(bufferRangeCount, pBufferRanges);
    else return VK_INCOMPLETE;
}

VkResult VKAPI_CALL vezCreateBufferView(VkDevice device, const VezBufferViewCreateInfo* pCreateInfo, VkBufferView* pView)
{
    // Lookup Device object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (!deviceImpl) return VK_INCOMPLETE;

    // Lookup Buffer object handle.
    auto bufferImpl = vez::ObjectLookup::GetImplBuffer(pCreateInfo->buffer);

    // If Buffer class object was not found, assume the VkBuffer handle comes from native Vulkan.
    // Create a new proxy Buffer class object and add it to the object lookup.
    if (!bufferImpl) bufferImpl = ImportVkBuffer(deviceImpl, pCreateInfo->buffer);
    
    // Create the buffer view.
    vez::BufferView* bufferViewImpl = nullptr;
    auto result = vez::BufferView::Create(bufferImpl, pCreateInfo->pNext, pCreateInfo->format, pCreateInfo->offset, pCreateInfo->range, &bufferViewImpl);
    if (result != VK_SUCCESS)
        return result;

    // Store native Vulkan handle in pView parameter.
    *pView = reinterpret_cast<VkBufferView>(bufferViewImpl->GetHandle());

    // Add to ObjectLookup.
    vez::ObjectLookup::AddObjectImpl(*pView, bufferViewImpl);

    // Return success.
    return VK_SUCCESS;
}

void VKAPI_CALL vezDestroyBufferView(VkDevice device, VkBufferView bufferView)
{
    // Lookup BufferView object handle.
    auto bufferViewImpl = vez::ObjectLookup::GetImplBufferView(bufferView);
    if (bufferViewImpl)
    {
        // Remove BufferView object from ObjectLookup and destroy the instance.
        vez::ObjectLookup::RemoveImplBufferView(bufferView);
        delete bufferViewImpl;
    }
}

VkResult VKAPI_CALL vezCreateImage(VkDevice device, VezMemoryFlags memFlags, const VezImageCreateInfo* pCreateInfo, VkImage* pImage)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (!deviceImpl)
        return VK_INCOMPLETE;

    // Create the image.
    vez::Image* imageImpl = nullptr;
    auto result = deviceImpl->CreateImage(memFlags, pCreateInfo, &imageImpl);
    if (result != VK_SUCCESS)
        return result;

    // Store native Vulkan handle in pImage parameter.
    *pImage = reinterpret_cast<VkImage>(imageImpl->GetHandle());

    // Add to ObjectLookup.
    vez::ObjectLookup::AddObjectImpl(*pImage, imageImpl);

    // Return success.
    return VK_SUCCESS;
}

void VKAPI_CALL vezDestroyImage(VkDevice device, VkImage image)
{
    // Lookup Image object handle.
    auto imageImpl = vez::ObjectLookup::GetImplImage(image);
    if (imageImpl)
    {
        // Remove the Image object from the ObjectLookup and destroy the instance.
        vez::ObjectLookup::RemoveImplImage(image);
        imageImpl->GetDevice()->DestroyImage(imageImpl);
    }
}

VkResult VKAPI_CALL vezImageSubData(VkDevice device, VkImage image, const VezImageSubDataInfo* pSubDataInfo, const void* pData)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (!deviceImpl)
        return VK_INCOMPLETE;

    // Lookup Image object handle.
    auto imageImpl = vez::ObjectLookup::GetImplImage(image);
    if (!imageImpl)
        return VK_INCOMPLETE;

    // Upload data to image.
    return imageImpl->GetDevice()->ImageSubData(imageImpl, pSubDataInfo, pData);
}

VkResult VKAPI_CALL vezCreateImageView(VkDevice device, const VezImageViewCreateInfo* pCreateInfo, VkImageView* pView)
{
    // Lookup Image object handle.
    auto imageImpl = vez::ObjectLookup::GetImplImage(pCreateInfo->image);
    if (!imageImpl)
        return VK_INCOMPLETE;

    // Create the buffer view.
    vez::ImageView* imageViewImpl = nullptr;
    auto result = vez::ImageView::Create(imageImpl, pCreateInfo->pNext, pCreateInfo->viewType, pCreateInfo->format, pCreateInfo->components, pCreateInfo->subresourceRange, &imageViewImpl);
    if (result != VK_SUCCESS)
        return result;

    // Store native Vulkan handle in pView parameter.
    *pView = reinterpret_cast<VkImageView>(imageViewImpl->GetHandle());

    // Add to ObjectLookup.
    vez::ObjectLookup::AddObjectImpl(*pView, imageViewImpl);

    // Return success.
    return VK_SUCCESS;
}

void VKAPI_CALL vezDestroyImageView(VkDevice device, VkImageView imageView)
{
    // Lookup ImageView object handle.
    auto imageViewImpl = vez::ObjectLookup::GetImplImageView(imageView);
    if (!imageViewImpl)
        return;

    // Remove the ImageView object from ObjectLookup and destroy the instance.
    vez::ObjectLookup::RemoveImplImageView(imageView);
    delete imageViewImpl;
}

VkResult VKAPI_CALL vezCreateFramebuffer(VkDevice device, const VezFramebufferCreateInfo* pCreateInfo, VezFramebuffer* pFramebuffer)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (!deviceImpl)
        return VK_INCOMPLETE;

    // Create the framebuffer.
    return vez::Framebuffer::Create(deviceImpl, pCreateInfo, reinterpret_cast<vez::Framebuffer**>(pFramebuffer));
}

void VKAPI_CALL vezDestroyFramebuffer(VkDevice device, VezFramebuffer framebuffer)
{
    delete reinterpret_cast<vez::Framebuffer*>(framebuffer);
}

VkResult VKAPI_CALL vezAllocateCommandBuffers(VkDevice device, const VezCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (!deviceImpl)
        return VK_INCOMPLETE;

    // Lookup the Queue object handle.
    auto queueImpl = vez::ObjectLookup::GetImplQueue(pAllocateInfo->queue);
    if (!queueImpl)
        return VK_INCOMPLETE;

    // Allocate the command buffers.
    std::vector<vez::CommandBuffer*> commandBuffers(pAllocateInfo->commandBufferCount);
    auto result = deviceImpl->AllocateCommandBuffers(queueImpl, pAllocateInfo->pNext, pAllocateInfo->commandBufferCount, commandBuffers.data());
    if (result != VK_SUCCESS)
        return result;

    // Copy native handles to pCommandBuffers and add to ObjectLookup.
    for (auto i = 0U; i < pAllocateInfo->commandBufferCount; ++i)
    {
        pCommandBuffers[i] = commandBuffers[i]->GetHandle();
        vez::ObjectLookup::AddObjectImpl(pCommandBuffers[i], commandBuffers[i]);
    }

    // Return success.
    return VK_SUCCESS;
}

void VKAPI_CALL vezFreeCommandBuffers(VkDevice device, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers)
{
    // Lookup object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (!deviceImpl)
        return;

    // Free command buffers and remove command buffers from ObjectLookup.
    for (auto i = 0U; i < commandBufferCount; ++i)
    {
        auto cmdBufferImpl = vez::ObjectLookup::GetImplCommandBuffer(pCommandBuffers[i]);
        deviceImpl->FreeCommandBuffers(1, &cmdBufferImpl);
        vez::ObjectLookup::RemoveImplCommandBuffer(pCommandBuffers[i]);
    }
}

VkResult VKAPI_CALL vezBeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags)
{
    auto cmdBufferImpl = vez::ObjectLookup::GetImplCommandBuffer(commandBuffer);
    if (!cmdBufferImpl)
        return VK_INCOMPLETE;

    s_pActiveCommandBuffer = cmdBufferImpl;
    return s_pActiveCommandBuffer->Begin(flags);
}

VkResult VKAPI_CALL vezEndCommandBuffer()
{
    if (!s_pActiveCommandBuffer)
        return VK_INCOMPLETE;

    auto result = s_pActiveCommandBuffer->End();
    s_pActiveCommandBuffer = nullptr;
    return result;
}

VkResult VKAPI_CALL vezResetCommandBuffer(VkCommandBuffer commandBuffer)
{
    auto cmdBufferImpl = vez::ObjectLookup::GetImplCommandBuffer(commandBuffer);
    if (!cmdBufferImpl) return VK_INCOMPLETE;
    else return cmdBufferImpl->Reset();
}

void VKAPI_CALL vezCmdBeginRenderPass(const VezRenderPassBeginInfo* pBeginInfo)
{
    s_pActiveCommandBuffer->CmdBeginRenderPass(pBeginInfo);
}

void VKAPI_CALL vezCmdNextSubpass()
{
    s_pActiveCommandBuffer->CmdNextSubpass();
}

void VKAPI_CALL vezCmdEndRenderPass()
{
    s_pActiveCommandBuffer->CmdEndRenderPass();
}

void VKAPI_CALL vezCmdBindPipeline(VezPipeline pipeline)
{
    s_pActiveCommandBuffer->CmdBindPipeline(reinterpret_cast<vez::Pipeline*>(pipeline));
}

void VKAPI_CALL vezCmdPushConstants(uint32_t offset, uint32_t size, const void* pValues)
{
    s_pActiveCommandBuffer->CmdPushConstants(offset, size, pValues);
}

void VKAPI_CALL vezCmdBindBuffer(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t arrayElement)
{
    // Lookup Buffer object handle.
    auto bufferImpl = vez::ObjectLookup::GetImplBuffer(buffer);

    // If Buffer class object was not found, assume the VkBuffer handle comes from native Vulkan.
    // Create a new proxy Buffer class object and add it to the object lookup.
    if (!bufferImpl) bufferImpl = ImportVkBuffer(s_pActiveCommandBuffer->GetPool()->GetDevice(), buffer);
    
    // Bind buffer.
    s_pActiveCommandBuffer->CmdBindBuffer(bufferImpl, offset, range, set, binding, arrayElement);
}

void VKAPI_CALL vezCmdBindBufferView(VkBufferView bufferView, uint32_t set, uint32_t binding, uint32_t arrayElement)
{
    // Lookup Buffer object handle.
    auto bufferViewImpl = vez::ObjectLookup::GetImplBufferView(bufferView);
    if (!bufferViewImpl)
        return;

    // Bind buffer view.
    s_pActiveCommandBuffer->CmdBindBufferView(bufferViewImpl, set, binding, arrayElement);
}

void VKAPI_CALL vezCmdBindImageView(VkImageView imageView, VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement)
{
    // Lookup ImageView object handle.
    auto imageViewImpl = vez::ObjectLookup::GetImplImageView(imageView);
    if (!imageViewImpl)
        return;

    // Bind buffer view.
    s_pActiveCommandBuffer->CmdBindImageView(imageViewImpl, sampler, set, binding, arrayElement);
}

void VKAPI_CALL vezCmdBindSampler(VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement)
{
    s_pActiveCommandBuffer->CmdBindSampler(sampler, set, binding, arrayElement);
}

void VKAPI_CALL vezCmdBindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffset)
{
    // Lookup Buffer object handles.
    std::vector<vez::Buffer*> buffers(bindingCount);
    for (auto i = 0U; i < bindingCount; ++i)
    {
        buffers[i] = vez::ObjectLookup::GetImplBuffer(pBuffers[i]);
        
        // If Buffer class object was not found, assume the VkBuffer handle comes from native Vulkan.
        // Create a new proxy Buffer class object and add it to the object lookup.
        if (!buffers[i]) buffers[i] = ImportVkBuffer(s_pActiveCommandBuffer->GetPool()->GetDevice(), pBuffers[i]);
    }

    // Bind vertex buffers.
    s_pActiveCommandBuffer->CmdBindVertexBuffers(firstBinding, bindingCount, buffers.data(), pOffset);
}

void VKAPI_CALL vezCmdBindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
    // Lookup Buffer object handle.
    auto bufferImpl = vez::ObjectLookup::GetImplBuffer(buffer);
    
    // If Buffer class object was not found, assume the VkBuffer handle comes from native Vulkan.
    // Create a new proxy Buffer class object and add it to the object lookup.
    if (!bufferImpl) bufferImpl = ImportVkBuffer(s_pActiveCommandBuffer->GetPool()->GetDevice(), buffer);

    // Bind index buffer.
    s_pActiveCommandBuffer->CmdBindIndexBuffer(bufferImpl, offset, indexType);
}

void VKAPI_CALL vezCmdSetVertexInputFormat(VezVertexInputFormat format)
{
    s_pActiveCommandBuffer->CmdSetVertexInputFormat(reinterpret_cast<vez::VertexInputFormat*>(format));
}

void VKAPI_CALL vezCmdSetViewportState(uint32_t viewportCount)
{
    s_pActiveCommandBuffer->CmdSetViewportState(viewportCount);
}

void VKAPI_CALL vezCmdSetInputAssemblyState(const VezInputAssemblyState* pStateInfo)
{
    s_pActiveCommandBuffer->CmdSetInputAssemblyState(pStateInfo);
}
void VKAPI_CALL vezCmdSetRasterizationState(const VezRasterizationState* pStateInfo)
{
    s_pActiveCommandBuffer->CmdSetRasterizationState(pStateInfo);
}

void VKAPI_CALL vezCmdSetMultisampleState(const VezMultisampleState* pStateInfo)
{
    s_pActiveCommandBuffer->CmdSetMultisampleState(pStateInfo);
}

void VKAPI_CALL vezCmdSetDepthStencilState(const VezDepthStencilState* pStateInfo)
{
    s_pActiveCommandBuffer->CmdSetDepthStencilState(pStateInfo);
}

void VKAPI_CALL vezCmdSetColorBlendState(const VezColorBlendState* pStateInfo)
{
    s_pActiveCommandBuffer->CmdSetColorBlendState(pStateInfo);
}

void VKAPI_CALL vezCmdSetViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports)
{
    s_pActiveCommandBuffer->CmdSetViewport(firstViewport, viewportCount, pViewports);
}

void VKAPI_CALL vezCmdSetScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors)
{
    s_pActiveCommandBuffer->CmdSetScissor(firstScissor, scissorCount, pScissors);
}

void VKAPI_CALL vezCmdSetLineWidth(float lineWidth)
{
    s_pActiveCommandBuffer->CmdSetLineWidth(lineWidth);
}

void VKAPI_CALL vezCmdSetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
{
    s_pActiveCommandBuffer->CmdSetDepthBias(depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

void VKAPI_CALL vezCmdSetBlendConstants(const float blendConstants[4])
{
    s_pActiveCommandBuffer->CmdSetBlendConstants(blendConstants);
}

void VKAPI_CALL vezCmdSetDepthBounds(float minDepthBounds, float maxDepthBounds)
{
    s_pActiveCommandBuffer->CmdSetDepthBounds(minDepthBounds, maxDepthBounds);
}

void VKAPI_CALL vezCmdSetStencilCompareMask(VkStencilFaceFlags faceMask, uint32_t compareMask)
{
    s_pActiveCommandBuffer->CmdSetStencilCompareMask(faceMask, compareMask);
}

void VKAPI_CALL vezCmdSetStencilWriteMask(VkStencilFaceFlags faceMask, uint32_t writeMask)
{
    s_pActiveCommandBuffer->CmdSetStencilWriteMask(faceMask, writeMask);
}

void VKAPI_CALL vezCmdSetStencilReference(VkStencilFaceFlags faceMask, uint32_t reference)
{
    s_pActiveCommandBuffer->CmdSetStencilReference(faceMask, reference);
}

void VKAPI_CALL vezCmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    s_pActiveCommandBuffer->CmdDraw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void VKAPI_CALL vezCmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    s_pActiveCommandBuffer->CmdDrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VKAPI_CALL vezCmdDrawIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    // Lookup Buffer object handle.
    auto bufferImpl = vez::ObjectLookup::GetImplBuffer(buffer);
    if (!bufferImpl)
        return;

    // Call draw indirect.
    s_pActiveCommandBuffer->CmdDrawIndirect(bufferImpl, offset, drawCount, stride);
}

void VKAPI_CALL vezCmdDrawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    // Lookup Buffer object handle.
    auto bufferImpl = vez::ObjectLookup::GetImplBuffer(buffer);
    
    // If Buffer class object was not found, assume the VkBuffer handle comes from native Vulkan.
    // Create a new proxy Buffer class object and add it to the object lookup.
    if (!bufferImpl) bufferImpl = ImportVkBuffer(s_pActiveCommandBuffer->GetPool()->GetDevice(), buffer);

    // Call draw indexed indirect.
    s_pActiveCommandBuffer->CmdDrawIndexedIndirect(bufferImpl, offset, drawCount, stride);
}

void VKAPI_CALL vezCmdDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    s_pActiveCommandBuffer->CmdDispatch(groupCountX, groupCountY, groupCountZ);
}

void VKAPI_CALL vezCmdDispatchIndirect(VkBuffer buffer, VkDeviceSize offset)
{
    // Lookup Buffer object handle.
    auto bufferImpl = vez::ObjectLookup::GetImplBuffer(buffer);
    
    // If Buffer class object was not found, assume the VkBuffer handle comes from native Vulkan.
    // Create a new proxy Buffer class object and add it to the object lookup.
    if (!bufferImpl) bufferImpl = ImportVkBuffer(s_pActiveCommandBuffer->GetPool()->GetDevice(), buffer);

    // Call dispatch indirect.
    s_pActiveCommandBuffer->CmdDispatchIndirect(bufferImpl, offset);
}

void VKAPI_CALL vezCmdCopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VezBufferCopy* pRegions)
{
    // Lookup Buffer object handle.
    auto srcBufferImpl = vez::ObjectLookup::GetImplBuffer(srcBuffer);
    
    // If Buffer class object was not found, assume the VkBuffer handle comes from native Vulkan.
    // Create a new proxy Buffer class object and add it to the object lookup.
    if (!srcBufferImpl) srcBufferImpl = ImportVkBuffer(s_pActiveCommandBuffer->GetPool()->GetDevice(), srcBuffer);

    // Lookup Buffer object handle.
    auto dstBufferImpl = vez::ObjectLookup::GetImplBuffer(dstBuffer);
    
    // If Buffer class object was not found, assume the VkBuffer handle comes from native Vulkan.
    // Create a new proxy Buffer class object and add it to the object lookup.
    if (!dstBufferImpl) dstBufferImpl = ImportVkBuffer(s_pActiveCommandBuffer->GetPool()->GetDevice(), dstBuffer);

    // Call copy buffer.
    s_pActiveCommandBuffer->CmdCopyBuffer(srcBufferImpl, dstBufferImpl, regionCount, pRegions);
}

void VKAPI_CALL vezCmdCopyImage(VkImage srcImage, VkImage dstImage, uint32_t regionCount, const VezImageCopy* pRegions)
{
    // Lookup Image object handles.
    auto srcImageImpl = vez::ObjectLookup::GetImplImage(srcImage);
    if (!srcImageImpl)
        return;

    auto dstImageImpl = vez::ObjectLookup::GetImplImage(dstImage);
    if (!dstImageImpl)
        return;

    // Call copy image.
    s_pActiveCommandBuffer->CmdCopyImage(srcImageImpl, dstImageImpl, regionCount, pRegions);
}

void VKAPI_CALL vezCmdBlitImage(VkImage srcImage, VkImage dstImage, uint32_t regionCount, const VezImageBlit* pRegions, VkFilter filter)
{
    // Lookup Image object handles.
    auto srcImageImpl = vez::ObjectLookup::GetImplImage(srcImage);
    if (!srcImageImpl)
        return;

    auto dstImageImpl = vez::ObjectLookup::GetImplImage(dstImage);
    if (!dstImageImpl)
        return;

    // Call blit image.
    s_pActiveCommandBuffer->CmdBlitImage(srcImageImpl, dstImageImpl, regionCount, pRegions, filter);
}

void VKAPI_CALL vezCmdCopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, uint32_t regionCount, const VezBufferImageCopy* pRegions)
{
    // Lookup Buffer and Image object handles.
    auto srcBufferImpl = vez::ObjectLookup::GetImplBuffer(srcBuffer);
    
    // If Buffer class object was not found, assume the VkBuffer handle comes from native Vulkan.
    // Create a new proxy Buffer class object and add it to the object lookup.
    if (!srcBufferImpl) srcBufferImpl = ImportVkBuffer(s_pActiveCommandBuffer->GetPool()->GetDevice(), srcBuffer);

    auto dstImageImpl = vez::ObjectLookup::GetImplImage(dstImage);
    if (!dstImageImpl)
        return;

    // Call copy buffer to image.
    s_pActiveCommandBuffer->CmdCopyBufferToImage(srcBufferImpl, dstImageImpl, regionCount, pRegions);
}

void VKAPI_CALL vezCmdCopyImageToBuffer(VkImage srcImage, VkBuffer dstBuffer, uint32_t regionCount, const VezBufferImageCopy* pRegions)
{
    // Lookup Buffer and Image object handles.
    auto srcImageImpl = vez::ObjectLookup::GetImplImage(srcImage);
    if (!srcImageImpl)
        return;

    auto dstBufferImpl = vez::ObjectLookup::GetImplBuffer(dstBuffer);
    
    // If Buffer class object was not found, assume the VkBuffer handle comes from native Vulkan.
    // Create a new proxy Buffer class object and add it to the object lookup.
    if (!dstBufferImpl) dstBufferImpl = ImportVkBuffer(s_pActiveCommandBuffer->GetPool()->GetDevice(), dstBuffer);

    // Call copy image to buffer.
    s_pActiveCommandBuffer->CmdCopyImageToBuffer(srcImageImpl, dstBufferImpl, regionCount, pRegions);
}

void VKAPI_CALL vezCmdUpdateBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData)
{
    // Lookup Buffer object handle.
    auto dstBufferImpl = vez::ObjectLookup::GetImplBuffer(dstBuffer);
    
    // If Buffer class object was not found, assume the VkBuffer handle comes from native Vulkan.
    // Create a new proxy Buffer class object and add it to the object lookup.
    if (!dstBufferImpl) dstBufferImpl = ImportVkBuffer(s_pActiveCommandBuffer->GetPool()->GetDevice(), dstBuffer);

    // Call update buffer.
    s_pActiveCommandBuffer->CmdUpdateBuffer(dstBufferImpl, dstOffset, dataSize, pData);
}

void VKAPI_CALL vezCmdFillBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data)
{
    // Lookup Buffer object handle.
    auto dstBufferImpl = vez::ObjectLookup::GetImplBuffer(dstBuffer);
    
    // If Buffer class object was not found, assume the VkBuffer handle comes from native Vulkan.
    // Create a new proxy Buffer class object and add it to the object lookup.
    if (!dstBufferImpl) dstBufferImpl = ImportVkBuffer(s_pActiveCommandBuffer->GetPool()->GetDevice(), dstBuffer);

    // Call fill buffer.
    s_pActiveCommandBuffer->CmdFillBuffer(dstBufferImpl, dstOffset, size, data);
}

void VKAPI_CALL vezCmdClearColorImage(VkImage image, const VkClearColorValue* pColor, uint32_t rangeCount, const VezImageSubresourceRange* pRanges)
{
    // Lookup Image object handle.
    auto imageImpl = vez::ObjectLookup::GetImplImage(image);
    if (!imageImpl)
        return;

    // Call clear color image.
    s_pActiveCommandBuffer->CmdClearColorImage(imageImpl, pColor, rangeCount, pRanges);
}

void VKAPI_CALL vezCmdClearDepthStencilImage(VkImage image, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VezImageSubresourceRange* pRanges)
{
    // Lookup Image object handle.
    auto imageImpl = vez::ObjectLookup::GetImplImage(image);
    if (!imageImpl)
        return;

    // Call clear depth stencil image.
    s_pActiveCommandBuffer->CmdClearDepthStencilImage(imageImpl, pDepthStencil, rangeCount, pRanges);
}

void VKAPI_CALL vezCmdClearAttachments(uint32_t attachmentCount, const VezClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects)
{
    s_pActiveCommandBuffer->CmdClearAttachments(attachmentCount, pAttachments, rectCount, pRects);
}

void VKAPI_CALL vezCmdResolveImage(VkImage srcImage, VkImage dstImage, uint32_t regionCount, const VezImageResolve* pRegions)
{
    // Lookup Image object handles.
    auto srcImageImpl = vez::ObjectLookup::GetImplImage(srcImage);
    if (!srcImageImpl)
        return;

    auto dstImageImpl = vez::ObjectLookup::GetImplImage(dstImage);
    if (!dstImageImpl)
        return;

    // Call clear color image.
    s_pActiveCommandBuffer->CmdResolveImage(srcImageImpl, dstImageImpl, regionCount, pRegions);
}

void VKAPI_CALL vezCmdSetEvent(VkEvent event, VkPipelineStageFlags stageMask)
{
    s_pActiveCommandBuffer->CmdSetEvent(event, stageMask);
}

void VKAPI_CALL vezCmdResetEvent(VkEvent event, VkPipelineStageFlags stageMask)
{
    s_pActiveCommandBuffer->CmdResetEvent(event, stageMask);
}