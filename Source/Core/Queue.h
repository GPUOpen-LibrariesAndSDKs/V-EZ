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

#include <queue>
#include <tuple>
#include <map>
#include "VEZ.h"

namespace vez
{
    class Device;
    class CommandBuffer;

    class Queue
    {
    public:
        Queue(Device* device, VkQueue queue, uint32_t queueFamilyIndex, uint32_t index, const VkQueueFamilyProperties& propertiesd);

        Device* GetDevice() const { return m_device; }

        VkQueue GetHandle() const { return m_handle; }

        uint32_t GetFamilyIndex() const { return m_queueFamilyIndex; }

        uint32_t GetIndex() const { return m_index; }

        VkQueueFlags GetFlags() const { return m_properties.queueFlags; }

        VkResult Submit(uint32_t submitCount, const VezSubmitInfo* pSubmits, VkFence* pFence);

        VkResult Present(const VezPresentInfo* pPresentInfo);

        VkResult WaitIdle();

    private:
        VkResult AcquireCommandBuffer(CommandBuffer** pCommandBuffer);

        Device* m_device = nullptr;
        VkQueue m_handle = VK_NULL_HANDLE;
        uint32_t m_queueFamilyIndex = 0;
        uint32_t m_index = 0;
        VkQueueFamilyProperties m_properties;

        // The purpose of this queue is to reuse command buffers by checking the fence status of the submission.
        // Each fence has a list of waitSemaphores it is responsible for releasing back to the SyncPrimitivesPool instance.
        std::queue<std::tuple<CommandBuffer*, VkFence>> m_presentCmdBuffers;

        // The final vkQueuePresent call has a single waitSemaphore, which must be released back to the SyncPrimitivesPool.
        // Since vkQueuePresent does not signal a fence, the list of swapchains and their indices will be used as a hash to determine
        // when to release the waitSemaphores.
        typedef std::vector<uint64_t> PresentHash;
        std::map<PresentHash, VkSemaphore> m_presentWaitSemaphores;
    };
}