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
#include <cmath>
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include "Utility/VkHelpers.h"
#include "Device.h"
#include "Image.h"
#include "ImageView.h"
#include "Framebuffer.h"
#include "RenderPassCache.h"

namespace vez
{
    static std::unordered_map<VkSampleCountFlagBits, uint8_t> sampleCountToBitField = {
        { VK_SAMPLE_COUNT_1_BIT, 0 },
        { VK_SAMPLE_COUNT_2_BIT, 1 },
        { VK_SAMPLE_COUNT_4_BIT, 2 },
        { VK_SAMPLE_COUNT_8_BIT, 3 },
        { VK_SAMPLE_COUNT_16_BIT, 4 },
        { VK_SAMPLE_COUNT_32_BIT, 5 },
        { VK_SAMPLE_COUNT_64_BIT, 6 }
    };

    static std::unordered_map<VkImageLayout, uint8_t> imageLayoutToBitField = {
        { VK_IMAGE_LAYOUT_UNDEFINED, 0 },
        { VK_IMAGE_LAYOUT_GENERAL, 1 },
        { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 2 },
        { VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 3 },
        { VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, 4 },
        { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 5 },
        { VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 6 },
        { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 7 },
        { VK_IMAGE_LAYOUT_PREINITIALIZED, 8 },
        { VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 9 }
    };

    static RenderPassHash GetHash(const RenderPassDesc* pDesc)
    {
        // Bit field structures for generating render pass hash.
        struct AttachmentDescriptionBitField
        {
            uint8_t format : 8;
            uint8_t samples : 4;
            uint8_t initialLayout : 4;
            uint8_t finalLayout : 4;
            uint8_t loadOp : 2;
            uint8_t storeOp : 2;
        };

        struct SubpassDescriptionBitField
        {
            uint8_t pipelineCount : 8;
        };

        struct SubpassDependencyBitField
        {
            uint32_t srcStageMask : 32;
            uint32_t dstStageMask : 32;
            uint32_t srcAccessMask : 32;
            uint32_t dstAccessMask : 32;
            uint16_t srcSubpass : 16;
            uint16_t dstSubpass : 16;
        };

        struct RenderPassBitField
        {
            uint8_t attachmentCount : 4;
            uint8_t subpassCount : 4;
            uint8_t dependencyCount : 8;
        };

        // Determine the total number of piplines used in render pass.
        uint32_t totalPipelineCount = 0;
        for (auto i = 0U; i < pDesc->subpasses.size(); ++i)
            totalPipelineCount += static_cast<uint32_t>(pDesc->subpasses[i].pipelineBindings.size());

        // Determine required hash size.
        size_t numBytes = sizeof(RenderPassBitField) +
            (sizeof(AttachmentDescriptionBitField) * pDesc->attachments.size()) +
            (sizeof(SubpassDescriptionBitField) * pDesc->subpasses.size()) +
            (sizeof(VkPipeline) * totalPipelineCount) +
            (sizeof(SubpassDependencyBitField) * pDesc->subpasses.size());

        // Get the number of 64-bit elements needed in the hash.
        uint32_t numElements = static_cast<uint32_t>(ceil(numBytes / 8.0f));

        // Allocate and compute the hash.
        RenderPassHash hash(numElements, 0ULL);
        auto baseAddress = reinterpret_cast<uint8_t*>(&hash[0]);
        auto renderPassBitField = reinterpret_cast<RenderPassBitField*>(&baseAddress[0]);
        auto attachmentDescriptions = reinterpret_cast<AttachmentDescriptionBitField*>(&renderPassBitField[1]);
        auto subpassDescriptions = reinterpret_cast<SubpassDescriptionBitField*>(&attachmentDescriptions[pDesc->attachments.size()]);
        auto pipelineBindings = reinterpret_cast<VkPipeline*>(&subpassDescriptions[pDesc->subpasses.size()]);
        auto subpassDependencies = reinterpret_cast<SubpassDependencyBitField*>(&pipelineBindings[totalPipelineCount]);

        // Fill in RenderPassBitField.
        renderPassBitField->attachmentCount = static_cast<uint8_t>(pDesc->attachments.size());
        renderPassBitField->subpassCount = static_cast<uint8_t>(pDesc->subpasses.size());
        renderPassBitField->dependencyCount = static_cast<uint8_t>(pDesc->subpasses.size());

        // Fill in AttachmentDescriptionBitField array.
        for (auto i = 0U; i < pDesc->attachments.size(); ++i)
        {
            attachmentDescriptions[i].format = static_cast<uint8_t>(pDesc->attachments[i].format);
            attachmentDescriptions[i].samples = sampleCountToBitField.at(pDesc->attachments[i].samples);
            attachmentDescriptions[i].initialLayout = imageLayoutToBitField.at(pDesc->attachments[i].initialLayout);
            attachmentDescriptions[i].finalLayout = imageLayoutToBitField.at(pDesc->attachments[i].finalLayout);
            attachmentDescriptions[i].loadOp = static_cast<uint8_t>(pDesc->attachments[i].loadOp);
            attachmentDescriptions[i].storeOp = static_cast<uint8_t>(pDesc->attachments[i].storeOp);
        }

        // Fill in SubpassDescriptionBitField and SubpassDependencyBitField arrays.
        for (auto i = 0U; i < pDesc->subpasses.size(); ++i)
        {
            // SubpassDescriptionBitField
            subpassDescriptions[i].pipelineCount = static_cast<uint8_t>(pDesc->subpasses[i].pipelineBindings.size());

            // Copy pipeline handles to hash.
            for (auto& entry : pDesc->subpasses[i].pipelineBindings)
            {
                *pipelineBindings = reinterpret_cast<VkPipeline>(entry.pipeline);
                ++pipelineBindings;
            }

            // SubpassDependencyBitField
            subpassDependencies[i].srcSubpass = static_cast<uint16_t>(pDesc->subpasses[i].dependency.srcSubpass);
            subpassDependencies[i].dstSubpass = static_cast<uint16_t>(pDesc->subpasses[i].dependency.dstSubpass);
            subpassDependencies[i].srcStageMask = static_cast<uint32_t>(pDesc->subpasses[i].dependency.srcStageMask);
            subpassDependencies[i].dstStageMask = static_cast<uint32_t>(pDesc->subpasses[i].dependency.dstStageMask);
            subpassDependencies[i].srcAccessMask = static_cast<uint32_t>(pDesc->subpasses[i].dependency.srcAccessMask);
            subpassDependencies[i].dstAccessMask = static_cast<uint32_t>(pDesc->subpasses[i].dependency.dstAccessMask);
        }

        // Return the resulting hash.
        return hash;
    }

    RenderPassCache::RenderPassCache(Device* device)
        : m_device(device)
    {

    }

    RenderPassCache::~RenderPassCache()
    {
        // Destroy any remaining render passes in the cache.
        for (auto it : m_renderPasses)
        {
            auto renderPass = it.second.renderPass;
            vkDestroyRenderPass(m_device->GetHandle(), renderPass->GetHandle(), nullptr);
            delete renderPass;
        }
    }

    VkResult RenderPassCache::CreateRenderPass(const RenderPassDesc* pDesc, RenderPass** ppRenderPass)
    {
        // Get the has for the given renderpass begin info.
        auto hash = GetHash(pDesc);

        // Lock access to cache.
        m_spinLock.Lock();

        // See if hash already exists in cache and return renderpass.
        bool foundInCache = false;
        auto it = m_renderPasses.find(hash);
        if (it != m_renderPasses.end())
        {
            ++it->second.references;
            *ppRenderPass = it->second.renderPass;
            foundInCache = true;
        }

        // Unlock access to cache.
        m_spinLock.Unlock();

        // If hash was found, exit.
        if (foundInCache)
            return VK_SUCCESS;

        // Get the bound framebuffer used in the render pass.
        auto framebuffer = reinterpret_cast<Framebuffer*>(pDesc->framebuffer);

        // Count the number of color attachments and find the index of the depth stencil attachment if it exists.
        uint32_t colorAttachmentCount = 0U;
        uint32_t depthStencilAttachmentIndex = VK_ATTACHMENT_UNUSED;
        for (auto i = 0U; i < pDesc->attachments.size(); ++i)
        {
            auto imageView = framebuffer->GetAttachment(i);
            if (IsDepthStencilFormat(imageView->GetFormat()))
                depthStencilAttachmentIndex = i;
            else
                ++colorAttachmentCount;
        }

        // All subpass description and attachment references stored in single large arrays.
        // (since Vulkan structs require const pointers to array data).
        std::vector<VkSubpassDescription> subpassDescriptions;
        std::vector<VkAttachmentReference> allInputAttachments;
        std::vector<VkAttachmentReference> allColorAttachments;
        std::vector<VkAttachmentReference> allResolveAttachments;
        std::vector<VkAttachmentReference> allDepthStencilAttachments;
        std::vector<uint32_t> allPreserveAttachments;

        // Construct all subpass descriptions.
        for (auto i = 0U; i < pDesc->subpasses.size(); ++i)
        {
            // Always set subpass pipeline bind point to graphics (required by Vulkan spec).
            VkSubpassDescription subpassDescription = {};
            subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

            // Fill in input attachments by iterating over all pipelines in the subpass and aggregate all input attachments.
            if (pDesc->subpasses[i].inputAttachments.size() > 0)
            {
                uint32_t maxInputAttachmentIndex = *pDesc->subpasses[i].inputAttachments.rbegin();
                for (auto k = 0U; k <= maxInputAttachmentIndex; ++k)
                {
                    allInputAttachments.push_back({ VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
                    if (pDesc->subpasses[i].inputAttachments.find(k) != pDesc->subpasses[i].inputAttachments.end())
                        allInputAttachments.back().attachment = k;

                    // Update subpass description (store vector size in pointer address and adjust in a second pass).
                    ++subpassDescription.inputAttachmentCount;
                    if (!subpassDescription.pInputAttachments)
                        subpassDescription.pInputAttachments = reinterpret_cast<const VkAttachmentReference*>(allInputAttachments.size());
                }
            }

            // Fill in color attachments.
            for (auto k = 0U; k < pDesc->attachments.size(); ++k)
            {
                // Skip depth stencil attachment.
                if (k == depthStencilAttachmentIndex)
                    continue;

                // Determine whether attachment was written to in current subpass and fill in attachment reference appropriately.
                allColorAttachments.push_back({ VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
                if (pDesc->subpasses[i].outputAttachments.find(k) != pDesc->subpasses[i].outputAttachments.end())
                    allColorAttachments.back().attachment = k;

                // Update subpass description (store vector size in pointer address and adjust in a second pass).
                ++subpassDescription.colorAttachmentCount;
                if (!subpassDescription.pColorAttachments)
                    subpassDescription.pColorAttachments = reinterpret_cast<const VkAttachmentReference*>(allColorAttachments.size());
            }

            // If a depth stencil buffer is present, fill in the attachment reference (store vector size in pointer address and adjust in a second pass).
            if (depthStencilAttachmentIndex != VK_ATTACHMENT_UNUSED)
            {
                allDepthStencilAttachments.push_back({ depthStencilAttachmentIndex, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL });
                subpassDescription.pDepthStencilAttachment = reinterpret_cast<const VkAttachmentReference*>(allDepthStencilAttachments.size());
            }

            //! Resolve and preserve attachments not supported at this time.
            
            // Store subpass description.
            subpassDescriptions.push_back(subpassDescription);
        }

        // Iterate back over vectors and set pointers to absolute memory addresses.
        for (auto& subpass : subpassDescriptions)
        {
            if (subpass.pInputAttachments)
                subpass.pInputAttachments = &allInputAttachments[reinterpret_cast<uint64_t>(subpass.pInputAttachments) - 1];

            if (subpass.pColorAttachments)
                subpass.pColorAttachments = &allColorAttachments[reinterpret_cast<uint64_t>(subpass.pColorAttachments) - 1];

            if (subpass.pDepthStencilAttachment)
                subpass.pDepthStencilAttachment = &allDepthStencilAttachments[reinterpret_cast<uint64_t>(subpass.pDepthStencilAttachment) - 1];

            // Consolidate layouts of entries between input and color attachments since the layouts must match for each slot index.
            uint32_t inputIndex, colorIndex;
            for (inputIndex = 0U, colorIndex = 0; inputIndex < subpass.inputAttachmentCount && colorIndex < subpass.colorAttachmentCount; ++inputIndex, ++colorIndex)
            {
                if (subpass.pInputAttachments[inputIndex].attachment != VK_ATTACHMENT_UNUSED && subpass.pColorAttachments[inputIndex].attachment == VK_ATTACHMENT_UNUSED)
                {
                    const_cast<VkAttachmentReference*>(subpass.pColorAttachments)[inputIndex].layout = subpass.pInputAttachments[inputIndex].layout;
                }
                else if (subpass.pInputAttachments[inputIndex].attachment == VK_ATTACHMENT_UNUSED && subpass.pColorAttachments[inputIndex].attachment != VK_ATTACHMENT_UNUSED)
                {
                    const_cast<VkAttachmentReference*>(subpass.pInputAttachments)[inputIndex].layout = subpass.pColorAttachments[inputIndex].layout;
                }
            }
        }

        // Copy subpass dependencies.
        std::vector<VkSubpassDependency> subpassDependencies;
        for (auto& subpassDesc : pDesc->subpasses)
            subpassDependencies.push_back(subpassDesc.dependency);

        // Add a final subpass dependency between the last subpass and external proceeding calls.
        VkSubpassDependency finalSubpassDependency = {};
        finalSubpassDependency.dependencyFlags = 0;
        finalSubpassDependency.srcSubpass = static_cast<uint32_t>(pDesc->subpasses.size() - 1);
        finalSubpassDependency.dstSubpass = VK_SUBPASS_EXTERNAL;
        finalSubpassDependency.srcStageMask = subpassDependencies.back().dstStageMask;
        finalSubpassDependency.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        finalSubpassDependency.srcAccessMask = subpassDependencies.back().dstAccessMask;
        finalSubpassDependency.dstAccessMask = 0;
        subpassDependencies.push_back(finalSubpassDependency);

        // Finally create Vulkan render pass object.
        VkRenderPassCreateInfo renderPassCreateInfo = {};
        renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(pDesc->attachments.size());
        renderPassCreateInfo.pAttachments = pDesc->attachments.data();
        renderPassCreateInfo.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
        renderPassCreateInfo.pSubpasses = subpassDescriptions.data();
        renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
        renderPassCreateInfo.pDependencies = subpassDependencies.data();

        VkRenderPass handle = VK_NULL_HANDLE;
        auto result = vkCreateRenderPass(m_device->GetHandle(), &renderPassCreateInfo, nullptr, &handle);
        if (result == VK_SUCCESS)
        {
            // Create a new RenderPassAllocation entry in the cache.
            m_spinLock.Lock();
            RenderPassAllocation rpa = { new RenderPass(hash, handle, colorAttachmentCount), 1 };
            m_renderPasses.emplace(hash, rpa);
            m_spinLock.Unlock();

            // Store new renderpass instance in pRenderPass.
            *ppRenderPass = rpa.renderPass;
        }

        // Return result.
        return static_cast<VkResult>(VK_SUCCESS);
    }

    void RenderPassCache::DestroyRenderPass(RenderPass* renderPass)
    {
        // Lock access to cache.
        m_spinLock.Lock();

        // Find the renderpass in the cache using it's hash.
        bool destroy = false;
        auto it = m_renderPasses.find(renderPass->GetHash());
        if (it != m_renderPasses.end())
        {
            if (it->second.references > 0)
                --it->second.references;
        }

        // Unlock access to cache.
        m_spinLock.Unlock();
    }

    void RenderPassCache::DestroyUnusedRenderPasses()
    {
        // Lock access to cache.
        m_spinLock.Lock();

        // Iterate over all render passes that no longer have any references.
        auto it = m_renderPasses.begin();
        while (it != m_renderPasses.end())
        {
            if (it->second.references == 0)
            {
                vkDestroyRenderPass(m_device->GetHandle(), it->second.renderPass->GetHandle(), nullptr);
                delete it->second.renderPass;
                it = m_renderPasses.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // Unlock access to cache.
        m_spinLock.Unlock();
    }
}