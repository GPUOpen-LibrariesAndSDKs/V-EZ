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

struct VmaAllocation_T;
typedef struct VmaAllocation_T* VmaAllocation;

namespace vez
{
    class Device;

    class Buffer
    {
    public:
        static Buffer* CreateFromHandle(Device* device, const VezBufferCreateInfo* pCreateInfo, VkBuffer buffer, VmaAllocation allocation);

        Device* GetDevice() { return m_device; }

        const VezBufferCreateInfo& GetCreateInfo() const { return m_createInfo; }

        VkBuffer GetHandle() { return m_handle; }

        VmaAllocation GetAllocation() { return m_allocation; }

    private:
        Device* m_device = nullptr;
        VezBufferCreateInfo m_createInfo;
        VkBuffer m_handle = VK_NULL_HANDLE;
        VmaAllocation m_allocation = VK_NULL_HANDLE;
    };
}