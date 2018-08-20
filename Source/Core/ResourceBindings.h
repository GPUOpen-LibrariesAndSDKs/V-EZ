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

#include <map>
#include <unordered_map>
#include "VEZ.h"

namespace vez
{
    class Buffer;
    class BufferView;
    class ImageView;

    struct BindingInfo
    {
        VkDeviceSize offset;
        VkDeviceSize range;
        Buffer* pBuffer;
        BufferView* pBufferView;
        ImageView* pImageView;
        VkSampler sampler;
        bool dirty;
    };

    typedef std::map<uint32_t, BindingInfo> ArrayBindings;

    struct SetBindings
    {
        std::unordered_map<uint32_t, ArrayBindings> bindings;
        bool dirty;
    };

    class ResourceBindings
    {
    public:
        bool IsDirty() const { return m_dirty; }

        const std::unordered_map<uint32_t, SetBindings>& GetSetBindings() { return m_setBindings; }

        void ClearDirtyBit() { m_dirty = false; }

        void Clear(uint32_t set);

        void Reset();

        void BindBuffer(Buffer* pBuffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t arrayElement);
        void BindBufferView(BufferView* pBufferView, uint32_t set, uint32_t binding, uint32_t arrayElement);
        void BindImageView(ImageView* pImageView, VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement);
        void BindSampler(VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement);

    private:
        void Bind(uint32_t set, uint32_t binding, uint32_t arrayElement, const BindingInfo& info);

        std::unordered_map<uint32_t, SetBindings> m_setBindings;
        bool m_dirty = false;
    };    
}