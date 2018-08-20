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
#include "Device.h"
#include "Buffer.h"
#include "BufferView.h"

namespace vez
{
    VkResult BufferView::Create(Buffer* pBuffer, const void* pNext, VkFormat format, VkDeviceSize offset, VkDeviceSize range, BufferView** pBufferView)
    {
        // Create a new Vulkan image view.
        VkBufferViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        createInfo.pNext = pNext;
        createInfo.buffer = pBuffer->GetHandle();
        createInfo.format = static_cast<VkFormat>(format);
        createInfo.offset = offset;
        createInfo.range = range;

        VkBufferView handle = VK_NULL_HANDLE;
        auto result = vkCreateBufferView(pBuffer->GetDevice()->GetHandle(), &createInfo, nullptr, &handle);
        if (result != VK_SUCCESS)
            return result;

        // Create a new BufferView class instance.
        BufferView* bufferView = new BufferView;
        bufferView->m_device = pBuffer->GetDevice();
        bufferView->m_buffer = pBuffer;
        bufferView->m_format = format;
        bufferView->m_offset = offset;
        bufferView->m_range = range;
        bufferView->m_handle = handle;

        // Copy object handle to parameter.
        *pBufferView = bufferView;

        // Return success.
        return VK_SUCCESS;
    }

    BufferView::~BufferView()
    {
        if (m_handle)
            vkDestroyBufferView(m_device->GetHandle(), m_handle, nullptr);
    }}