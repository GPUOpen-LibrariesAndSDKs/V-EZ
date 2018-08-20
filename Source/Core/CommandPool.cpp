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
#include <vector>
#include "Device.h"
#include "CommandBuffer.h"
#include "CommandPool.h"

namespace vez
{
    VkResult CommandPool::Create(Device* device, uint32_t queueFamilyIndex, CommandPool** pPool)
    {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndex;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VkCommandPool handle = VK_NULL_HANDLE;
        auto result = vkCreateCommandPool(device->GetHandle(), &poolInfo, nullptr, &handle);
        if (result != VK_SUCCESS)
            return result;

        *pPool = new CommandPool;
        (*pPool)->m_device = device;
        (*pPool)->m_queueFamilyIndex = queueFamilyIndex;
        (*pPool)->m_handle = handle;
        return VK_SUCCESS;
    }

    CommandPool::~CommandPool()
    {
        if (m_handle)
            vkDestroyCommandPool(m_device->GetHandle(), m_handle, nullptr);
    }

    VkResult CommandPool::AllocateCommandBuffers(const void* pNext, uint32_t commandBufferCount, VkCommandBuffer* pCommandBuffers)
    {
        // Safe guard access to internal resources across threads.
        m_spinLock.Lock();

        // Allocate a new command buffer.
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.pNext = pNext;
        allocInfo.commandPool = m_handle;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = commandBufferCount;
        auto result = vkAllocateCommandBuffers(m_device->GetHandle(), &allocInfo, pCommandBuffers);

        // Unlock access to internal resources.
        m_spinLock.Unlock();

        // Return result.
        return result;
    }

    void CommandPool::FreeCommandBuffers(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers)
    {
        // Safe guard access to internal resources across threads.
        m_spinLock.Lock();
        vkFreeCommandBuffers(m_device->GetHandle(), m_handle, commandBufferCount, pCommandBuffers);
        m_spinLock.Unlock();
    }
}