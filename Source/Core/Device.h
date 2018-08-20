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
#pragma once
#include <memory>
#include <vector>
#include <queue>
#include <unordered_map>
#include <thread>
#include <atomic>
#include "Utility/Macros.h"
#include "Utility/SpinLock.h"
#include "VEZ.h"

struct VmaAllocator_T;
typedef struct VmaAllocator_T* VmaAllocator;

namespace vez
{
    class PhysicalDevice;
    class Queue;
    class Swapchain;
    class CommandPool;
    class CommandBuffer;
    class SyncPrimitivesPool;
    class PipelineCache;
    class DescriptorSetLayoutCache;
    class RenderPassCache;
    class Framebuffer;
    class Buffer;
    class Image;
    class Fence;

    typedef std::vector<Queue*> QueueFamily;
    typedef std::unordered_map<Queue*, CommandPool*> QueueCommandPools;

    class Device
    {
    public:
        static VkResult Create(PhysicalDevice* pPhysicalDevice, const VezDeviceCreateInfo* pCreateInfo, Device** ppDevice);

        static void Destroy(Device* pDevice);

        PhysicalDevice* GetPhysicalDevice() const { return m_physicalDevice; }

        VkDevice GetHandle() const { return m_handle; }

        const std::vector<QueueFamily>& GetQueueFamilies() const { return m_queues; }

        SyncPrimitivesPool* GetSyncPrimitivesPool() { return m_syncPrimitivesPool; }

        PipelineCache* GetPipelineCache() { return m_pipelineCache; }

        DescriptorSetLayoutCache* GetDescriptorSetLayoutCache() { return m_descriptorSetLayoutCache; }

        RenderPassCache* GetRenderPassCache() { return m_renderPassCache; }

        Queue* GetQueue(uint32_t queueFamilyIndex, uint32_t queueIndex);

        Queue* GetQueueByFlags(VkQueueFlags queueFlags, uint32_t queueIndex);

        CommandPool* GetCommandPool(Queue* queue);

        VkResult AllocateCommandBuffers(Queue* pQueue, const void* pNext, uint32_t commandBufferCount, CommandBuffer** ppCommandBuffers, uint64_t streamEncoderBlockSize = MEGABYTES(8));

        void FreeCommandBuffers(uint32_t commandBufferCount, CommandBuffer** ppCommandBuffers);

        VkResult WaitIdle();

        VkResult SetVSync(VkBool32 enabled);

        VkResult CreateBuffer(VezMemoryFlags memFlags, const VezBufferCreateInfo* pCreateInfo, Buffer** ppBuffer);

        void DestroyBuffer(Buffer* pBuffer);

        VkResult BufferSubData(Buffer* pBuffer, VkDeviceSize offset, VkDeviceSize size, const void* pData);

        VkResult MapBuffer(Buffer* pBuffer, VkDeviceSize offset, VkDeviceSize size, void** ppData);

        void UnmapBuffer(Buffer* pBuffer);

        VkResult FlushMappedBufferRanges(uint32_t bufferRangeCount, const VezMappedBufferRange* pBufferRanges);

        VkResult InvalidateMappedBufferRanges(uint32_t bufferRangeCount, const VezMappedBufferRange* pBufferRanges);

        VkResult CreateImage(VezMemoryFlags memFlags, const VezImageCreateInfo* pCreateInfo, Image** ppImage);

        void DestroyImage(Image* pImage);

        VkResult ImageSubData(Image* pImage, const VezImageSubDataInfo* pSubDataInfo, const void* pData);

        VkResult Present(Queue* pQueue, Image* pImage, const VezPresentInfo* pPresentInfo);

        void TransitionImageLayout(Image* pImage, VkImageLayout oldLayout, VkImageLayout newLayout);

        void QueueSubmission(Fence* pFence);

        void DestroyFence(Fence* pFence);

        void DestroySemaphore(VkSemaphore semaphore);

    private:
        ~Device();

        VkResult UncompressedImageSubData(Image* pImage, const VezImageSubDataInfo* pSubDataInfo, const void* pData);

        VkResult CompressedImageSubData(Image* pImage, const VezImageSubDataInfo* pSubDataInfo, const void* pData);

        CommandBuffer* GetOneTimeSubmitCommandBuffer();

        PhysicalDevice* m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_handle = VK_NULL_HANDLE;
        VezDeviceCreateInfo m_createInfo = {};
        VmaAllocator m_memAllocator = VK_NULL_HANDLE;
        std::vector<QueueFamily> m_queues = {};
        std::unordered_map<std::thread::id, QueueCommandPools> m_commandPools;
        std::unordered_map<std::thread::id, CommandBuffer*> m_oneTimeSubmitCommandBuffers;
        SyncPrimitivesPool* m_syncPrimitivesPool = nullptr;
        PipelineCache* m_pipelineCache = nullptr;
        DescriptorSetLayoutCache* m_descriptorSetLayoutCache = nullptr;
        RenderPassCache* m_renderPassCache = nullptr;
        Buffer* m_pinnedMemoryBuffer = nullptr;
        void* m_pinnedMemoryPtr = nullptr;
        VkBool32 m_vsyncEnabled = false;

        std::queue<Fence*> m_trackedFences;
        SpinLock m_trackedFencesLock;

        std::atomic<std::uint32_t> m_fencesQueuedRunningCount { 0U };
        const uint32_t m_fencesQueuedUntilTrackedFencesEval = 3U;
        const uint32_t m_fencesQueuedUntilRenderPassCacheEval = 5000U;
    };
}