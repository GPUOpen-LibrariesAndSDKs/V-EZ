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
#include "VEZ_ext.h"
#include "Utility/ObjectLookup.h"
#include "Core/Device.h"
#include "Core/CommandBuffer.h"
#include "Core/Image.h"

VkResult VKAPI_CALL vezImportVkImage(VkDevice device, VkImage image, VkFormat format, VkExtent3D extent, VkSampleCountFlagBits samples, VkImageLayout imageLayout)
{
    // Lookup device object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (!deviceImpl)
        return VK_INCOMPLETE;

    // Wrap the Vulkan image object handle.
    VezImageCreateInfo createInfo = {};
    createInfo.format = format;
    createInfo.samples = samples;
    createInfo.extent = extent;    
    auto imageImpl = vez::Image::CreateFromHandle(deviceImpl, &createInfo, imageLayout, image, VK_NULL_HANDLE);

    // Add to ObjectLookup.
    vez::ObjectLookup::AddObjectImpl(image, imageImpl);

    // Return success.
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vezRemoveImportedVkImage(VkDevice device, VkImage image)
{
    // Lookup device object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (!deviceImpl)
        return VK_INCOMPLETE;

    // Remove object.
    vez::ObjectLookup::RemoveImplImage(image);
    return VK_SUCCESS;
}

VkResult VKAPI_CALL vezGetImageLayout(VkDevice device, VkImage image, VkImageLayout* pImageLayout)
{
    // Lookup device object handle.
    auto deviceImpl = vez::ObjectLookup::GetImplDevice(device);
    if (!deviceImpl)
        return VK_INCOMPLETE;

    // Lookup image object handle.
    auto imageImpl = vez::ObjectLookup::GetImplImage(image);
    if (!imageImpl)
        return VK_INCOMPLETE;

    // Get default image layout.
    *pImageLayout = imageImpl->GetDefaultImageLayout();

    // Return success.
    return VK_SUCCESS;
}