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

#include "VEZ.h"
#include "Utility/SpinLock.h"

namespace vez
{
    class Device;

    class CommandPool
    {
    public:
        static VkResult Create(Device* device, uint32_t queueFamilyIndex, CommandPool** pPool);

        ~CommandPool();

        Device* GetDevice() { return m_device; }

        uint32_t GetQueueFamilyIndex() const { return m_queueFamilyIndex; }

        VkCommandPool GetHandle() const { return m_handle; }

        VkResult AllocateCommandBuffers(const void* pNext, uint32_t commandBufferCount, VkCommandBuffer* pCommandBuffers);

        void FreeCommandBuffers(uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers);

    private:
        Device* m_device = nullptr;
        uint32_t m_queueFamilyIndex = 0;
        VkCommandPool m_handle = VK_NULL_HANDLE;
        SpinLock m_spinLock;
    };    
}