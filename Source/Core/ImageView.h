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

namespace vez
{
    class Device;
    class Image;

    class ImageView
    {
    public:
        static VkResult Create(Image* pImage, const void* pNext, VkImageViewType viewType, VkFormat format, VkComponentMapping components, VezImageSubresourceRange subresourceRange, ImageView** ppView);

        ~ImageView();

        Device* GetDevice() { return m_device; }

        Image* GetImage() { return m_image; }

        VkImageViewType GetViewType() { return m_viewType; }

        VkFormat GetFormat() { return m_format; }

        VkComponentMapping GetComponentMapping() { return m_components; }

        const VezImageSubresourceRange& GetSubresourceRange() { return m_subresourceRange; }

        VkImageView& GetHandle() { return m_handle; }

    private:
        Device* m_device;
        Image* m_image;
        VkImageViewType m_viewType;
        VkFormat m_format;
        VkComponentMapping m_components;
        VezImageSubresourceRange m_subresourceRange;
        VkImageView m_handle = VK_NULL_HANDLE;
    };    
}