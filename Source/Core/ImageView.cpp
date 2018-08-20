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
#include "Utility/VkHelpers.h"
#include "Device.h"
#include "Image.h"
#include "ImageView.h"

namespace vez
{
    VkResult ImageView::Create(Image* pImage, const void* pNext, VkImageViewType viewType, VkFormat format, VkComponentMapping components, VezImageSubresourceRange subresourceRange, ImageView** ppView)
    {
        // Create a new Vulkan image view.
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.pNext = pNext;
        createInfo.image = pImage->GetHandle();
        createInfo.viewType = static_cast<VkImageViewType>(viewType);
        createInfo.format = static_cast<VkFormat>(format);
        memcpy(&createInfo.components, &components, sizeof(VkComponentMapping));
        createInfo.subresourceRange.aspectMask = GetImageAspectFlags(format);
        createInfo.subresourceRange.baseMipLevel = subresourceRange.baseMipLevel;
        createInfo.subresourceRange.levelCount = subresourceRange.levelCount;
        createInfo.subresourceRange.baseArrayLayer = subresourceRange.baseArrayLayer;
        createInfo.subresourceRange.layerCount = subresourceRange.layerCount;

        VkImageView handle = VK_NULL_HANDLE;
        auto result = vkCreateImageView(pImage->GetDevice()->GetHandle(), &createInfo, nullptr, &handle);
        if (result != VK_SUCCESS)
            return result;

        // Create a new ImageView class instance.
        ImageView* imageView = new ImageView;
        imageView->m_device = pImage->GetDevice();
        imageView->m_image = pImage;
        imageView->m_viewType = viewType;
        imageView->m_format = format;
        memcpy(&imageView->m_components, &components, sizeof(VkComponentMapping));
        memcpy(&imageView->m_subresourceRange, &subresourceRange, sizeof(VezImageSubresourceRange));
        imageView->m_handle = handle;

        // Copy object handle to parameter.
        *ppView = imageView;

        // Return success.
        return VK_SUCCESS;
    }

    ImageView::~ImageView()
    {
        if (m_handle)
            vkDestroyImageView(m_device->GetHandle(), m_handle, nullptr);
    }    
}