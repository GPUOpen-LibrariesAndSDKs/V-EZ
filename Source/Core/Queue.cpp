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
#include <vector>
#include <cstring>
#include "Utility/ObjectLookup.h"
#include "Device.h"
#include "Swapchain.h"
#include "CommandBuffer.h"
#include "Image.h"
#include "SyncPrimitivesPool.h"
#include "Fence.h"
#include "Queue.h"

namespace vez
{
    Queue::Queue(Device* device, VkQueue queue, uint32_t queueFamilyIndex, uint32_t index, const VkQueueFamilyProperties& properties)
        : m_device(device)
        , m_handle(queue)
        , m_queueFamilyIndex(queueFamilyIndex)
        , m_index(index)
        , m_properties(properties)
    {

    }

    VkResult Queue::Submit(uint32_t submitCount, const VezSubmitInfo* pSubmits, VkFence* pFence)
    {
        // Create a fence for the submission if calling application requests one.
        auto syncPrimitivesPool = m_device->GetSyncPrimitivesPool();
        VkFence fence = VK_NULL_HANDLE;
        auto result = syncPrimitivesPool->AcquireFence(&fence);
        if (result != VK_SUCCESS)
            return result;

        // Pre-allocate space for all data structures.
        auto totalCommandBuffers = 0U;
        auto totalWaitSemaphores = 0U;
        auto totalSignalSemaphores = 0U;
        for (auto i = 0U; i < submitCount; ++i)
        {
            totalCommandBuffers += pSubmits[i].commandBufferCount;
            totalWaitSemaphores += pSubmits[i].waitSemaphoreCount;
            totalSignalSemaphores += pSubmits[i].signalSemaphoreCount;
        }

        // Fill native Vulkan data structures.
        std::vector<VkSubmitInfo> submitInfos(submitCount);
        std::vector<VkSemaphore> waitSemaphores(totalWaitSemaphores);
        std::vector<VkPipelineStageFlags> waitDstStageMasks(totalWaitSemaphores);
        std::vector<VkSemaphore> signalSemaphores(totalSignalSemaphores);

        VkSemaphore* pNextWaitSemaphore = nullptr;
        VkPipelineStageFlags* pNextWaitDstStageMask = nullptr;
        VkSemaphore* pNextSignalSemaphore = nullptr;

        if (waitSemaphores.size() > 0) pNextWaitSemaphore = &waitSemaphores[0];
        if (waitDstStageMasks.size() > 0) pNextWaitDstStageMask = &waitDstStageMasks[0];
        if (signalSemaphores.size() > 0) pNextSignalSemaphore = &signalSemaphores[0];

        // Iterate over all submissions.
        for (auto i = 0U; i < submitCount; ++i)
        {
            // Set submit info parameters to default values.
            submitInfos[i] = {};
            submitInfos[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfos[i].commandBufferCount = pSubmits[i].commandBufferCount;
            submitInfos[i].pCommandBuffers = pSubmits[i].pCommandBuffers;
           
            // Copy wait semaphores.
            for (auto k = 0U; k < pSubmits[i].waitSemaphoreCount; ++k)
            {
                if (!submitInfos[i].pWaitSemaphores)
                {
                    submitInfos[i].pWaitSemaphores = pNextWaitSemaphore;
                    submitInfos[i].pWaitDstStageMask = pNextWaitDstStageMask;
                }

                *pNextWaitSemaphore = reinterpret_cast<VkSemaphore>(pSubmits[i].pWaitSemaphores[k]);
                *pNextWaitDstStageMask = static_cast<VkPipelineStageFlags>(pSubmits[i].pWaitDstStageMask[k]);

                ++pNextWaitSemaphore;
                ++pNextWaitDstStageMask;
                ++submitInfos[i].waitSemaphoreCount;
            }

            // Copy signal semaphores.
            for (auto k = 0U; k < pSubmits[i].signalSemaphoreCount; ++k)
            {
                if (!submitInfos[i].pSignalSemaphores)
                    submitInfos[i].pSignalSemaphores = pNextSignalSemaphore;

                VkSemaphore semaphore = VK_NULL_HANDLE;
                auto result = syncPrimitivesPool->AcquireSemaphore(1, &semaphore);
                if (result != VK_SUCCESS)
                    return result;

                *pNextSignalSemaphore = semaphore;
                pSubmits[i].pSignalSemaphores[k] = reinterpret_cast<VkSemaphore>(semaphore);

                ++pNextSignalSemaphore;
                ++submitInfos[i].signalSemaphoreCount;
            }
        }

        // Submit to the Vulkan queue.
        result = vkQueueSubmit(m_handle, static_cast<uint32_t>(submitInfos.size()), submitInfos.data(), fence);

        // Wrap fence in Fence class object and store references to all signal semaphores.
        auto fenceImpl = new Fence(fence, totalWaitSemaphores, waitSemaphores.data());
        ObjectLookup::AddObjectImpl(fence, fenceImpl);

        // Store reference to fence in application supplied pointer.
        if (pFence)
            *pFence = fence;

        // Inform the owner Device a fence has been created for the queue submission (if application is not manually managing it).
        if (!pFence)
            m_device->QueueSubmission(fenceImpl);

        // Return result.
        return result;
    }

    VkResult Queue::Present(const VezPresentInfo* pPresentInfo)
    {
        // Acquire image indices of each swapchain's next image.
        std::vector<VkSwapchainKHR> swapchains(pPresentInfo->swapchainCount, VK_NULL_HANDLE);
        std::vector<uint32_t> imageIndices(pPresentInfo->swapchainCount, 0U);
        std::vector<VkSemaphore> imageAcquiredSemaphores(pPresentInfo->swapchainCount, VK_NULL_HANDLE);
        for (auto i = 0U; i < pPresentInfo->swapchainCount; ++i)
        {
            auto swapchain = reinterpret_cast<Swapchain*>(pPresentInfo->pSwapchains[i]);
            auto result = swapchain->AcquireNextImage(&imageIndices[i], &imageAcquiredSemaphores[i]);
            if (result != VK_SUCCESS)
                return result;

            swapchains[i] = swapchain->GetHandle();
        }

        // Acquire a command buffer and fence for image blit operations.
        CommandBuffer* commandBuffer = nullptr;
        auto result = AcquireCommandBuffer(&commandBuffer);
        if (result != VK_SUCCESS)
            return result;

        // Begin command buffer recording.
        commandBuffer->Begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

        // Blit the source images to each of the swapchains' images.
        for (auto i = 0U; i < pPresentInfo->swapchainCount; ++i)
        {
            // Get class objects.
            auto srcImage = ObjectLookup::GetImplImage(pPresentInfo->pImages[i]);
            auto swapchain = reinterpret_cast<Swapchain*>(pPresentInfo->pSwapchains[i]);
            auto dstImage = swapchain->GetImage(imageIndices[i]);

            // If source image is multisampled, resolve it into the swapchain image.
            if (srcImage->GetCreateInfo().samples != VK_SAMPLE_COUNT_1_BIT)
            {
                VezImageResolve region = {};
                region.srcOffset = { 0, 0, 0 };
                region.dstOffset = { 0, 0, 0 };
                region.extent = srcImage->GetCreateInfo().extent;
                region.srcSubresource.mipLevel = 0;
                region.srcSubresource.baseArrayLayer = 0;
                region.srcSubresource.layerCount = 1;
                memcpy(&region.dstSubresource, &region.srcSubresource, sizeof(VezImageSubresourceLayers));
                commandBuffer->CmdResolveImage(srcImage, dstImage, 1, &region);
            }
            // Else do an image copy.
            else
            {
                VezImageCopy region = {};
                region.srcOffset = { 0, 0, 0 };
                region.dstOffset = { 0, 0, 0 };
                region.extent = srcImage->GetCreateInfo().extent;
                region.srcSubresource.mipLevel = 0;
                region.srcSubresource.baseArrayLayer = 0;
                region.srcSubresource.layerCount = 1;
                memcpy(&region.dstSubresource, &region.srcSubresource, sizeof(VezImageSubresourceLayers));
                commandBuffer->CmdCopyImage(srcImage, dstImage, 1, &region);
            }
        }

        // End command buffer recording.
        commandBuffer->End();

        // Aggregate wait semaphore lists.
        std::vector<VkSemaphore> waitSemaphores;
        std::vector<VkPipelineStageFlags> waitDstStageMasks;
        for (auto semaphore : imageAcquiredSemaphores)
        {
            waitSemaphores.push_back(semaphore);
            waitDstStageMasks.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        }

        for (auto i = 0U; i < pPresentInfo->waitSemaphoreCount; ++i)
        {
            waitSemaphores.push_back(pPresentInfo->pWaitSemaphores[i]);
            waitDstStageMasks.push_back(pPresentInfo->pWaitDstStageMask[i]);
        }

        // Submit command buffer to queue.
        auto cmdBufferHandle = commandBuffer->GetHandle();
        VezSubmitInfo submitInfo = {};
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmdBufferHandle;
        submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitDstStageMasks.data();

        std::vector<VkSemaphore> signalSemaphores(1 + pPresentInfo->signalSemaphoreCount, VK_NULL_HANDLE);
        submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
        submitInfo.pSignalSemaphores = signalSemaphores.data();
        
        VkFence fence = VK_NULL_HANDLE;
        result = Submit(1, &submitInfo, &fence);
        if (result != VK_SUCCESS)
            return result;
   
        // Push command buffer and fence onto queue.
        m_presentCmdBuffers.push(std::make_tuple(commandBuffer, fence));

        // Queue present.
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &signalSemaphores[0];
        presentInfo.swapchainCount = static_cast<uint32_t>(swapchains.size());
        presentInfo.pSwapchains = swapchains.data();
        presentInfo.pImageIndices = imageIndices.data();
        result = vkQueuePresentKHR(m_handle, &presentInfo);

        // Copy signal semaphores back to VezPresentInfo struct.
        for (auto i = 0U; i < pPresentInfo->signalSemaphoreCount; ++i)
            pPresentInfo->pSignalSemaphores[i] = signalSemaphores[1 + i];

        // Generate hash from set of swapchains and their corresponding image indices.
        PresentHash hash(swapchains.size() + static_cast<uint32_t>(ceilf(imageIndices.size() / 2.0f)), 0ULL);
        memcpy(hash.data(), swapchains.data(), sizeof(uint64_t) * swapchains.size());
        memcpy(hash.data() + swapchains.size(), imageIndices.data(), sizeof(uint32_t) * imageIndices.size());

        // Free any previous waitSemaphore associated with these swapchains and image indices.
        auto it = m_presentWaitSemaphores.find(hash);
        if (it != m_presentWaitSemaphores.end())
            m_device->GetSyncPrimitivesPool()->ReleaseSemaphores(1, &it->second);

        // Store hash with new waitSemaphore.
        m_presentWaitSemaphores[hash] = signalSemaphores[0];

        // Return present result.
        return result;
    }

    VkResult Queue::WaitIdle()
    {
        return static_cast<VkResult>(vkQueueWaitIdle(m_handle));
    }

    VkResult Queue::AcquireCommandBuffer(CommandBuffer** pCommandBuffer)
    {
        // Check back of queue to see if there are any free command buffers.
        if (m_presentCmdBuffers.size() > 0)
        {
            auto fence = std::get<1>(m_presentCmdBuffers.front());
            auto status = vkGetFenceStatus(m_device->GetHandle(), fence);
            if (status == VK_SUCCESS)
            {
                m_device->DestroyFence(vez::ObjectLookup::GetImplFence(fence));
                *pCommandBuffer = std::get<0>(m_presentCmdBuffers.front());
                m_presentCmdBuffers.pop();
                return VK_SUCCESS;
            }
        }

        // Create a new command buffer.
        auto result = m_device->AllocateCommandBuffers(this, nullptr, 1, pCommandBuffer);
        if (result != VK_SUCCESS)
            return result;

        // Return success.
        return VK_SUCCESS;
    }
}