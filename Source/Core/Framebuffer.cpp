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
#include <array>
#include <vector>
#include "Utility/VkHelpers.h"
#include "Utility/ObjectLookup.h"
#include "Device.h"
#include "Image.h"
#include "ImageView.h"
#include "RenderPassCache.h"
#include "Framebuffer.h"

namespace vez
{
    VkResult Framebuffer::Create(Device* device, const VezFramebufferCreateInfo* pCreateInfo, Framebuffer** ppFramebuffer)
    {
        // Iterate over attachments and make sure each ImageView exists in ObjectLookup.
        std::vector<ImageView*> attachments;
        for (auto i = 0U; i < pCreateInfo->attachmentCount; ++i)
        {
            auto imageView = ObjectLookup::GetImplImageView(pCreateInfo->pAttachments[i]);
            if (!imageView)
                return VK_INCOMPLETE;

            attachments.push_back(imageView);
        }

        // Create a Framebuffer class instance.
        Framebuffer* framebuffer = new Framebuffer;
        framebuffer->m_device = device;
        framebuffer->m_pNext = pCreateInfo->pNext;
        framebuffer->m_width = pCreateInfo->width;
        framebuffer->m_height = pCreateInfo->height;
        framebuffer->m_layers = pCreateInfo->layers;
        framebuffer->m_attachments = std::move(attachments);

        // Copy object address to parameter.
        *ppFramebuffer = framebuffer;

        // Return success.
        return VK_SUCCESS;
    }

    Framebuffer::~Framebuffer()
    {
        for (auto itr : m_cache)
            vkDestroyFramebuffer(m_device->GetHandle(), itr.second, nullptr);
    }

    VkFramebuffer Framebuffer::GetHandle(RenderPass* pRenderPass)
    {
        // Make thread-safe.
        m_spinLock.Lock();

        // See if handle already exists for given render pass.
        VkFramebuffer handle = VK_NULL_HANDLE;
        auto itr = m_cache.find(pRenderPass);
        if (itr != m_cache.cend())
        {
            handle = itr->second;
        }
        else
        {
            // Get the native Vulkan ImageView handles from the attachment list.
            std::vector<VkImageView> attachments(m_attachments.size());
            for (auto i = 0U; i < m_attachments.size(); ++i)
                attachments[i] = m_attachments[i]->GetHandle();

            // Create the Vulkan framebuffer.
            VkFramebufferCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            createInfo.pNext = m_pNext;
            createInfo.renderPass = pRenderPass->GetHandle();
            createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            createInfo.pAttachments = attachments.data();
            createInfo.width = m_width;
            createInfo.height = m_height;
            createInfo.layers = m_layers;

            auto result = vkCreateFramebuffer(m_device->GetHandle(), &createInfo, nullptr, &handle);
            if (result == VK_SUCCESS)
                m_cache.emplace(pRenderPass, handle);
        }

        // Unlock access.
        m_spinLock.Unlock();

        // Return the result.
        return handle;
    }

    VkExtent2D Framebuffer::GetExtents()
    {
        return { m_width, m_height };
    }

    ImageView* Framebuffer::GetAttachment(uint32_t attachmentIndex)
    {
        if (attachmentIndex < m_attachments.size()) return m_attachments[attachmentIndex];
        else return VK_NULL_HANDLE;
    }    
}