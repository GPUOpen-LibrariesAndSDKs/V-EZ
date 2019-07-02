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
#include <unordered_set>
#include "Utility/VkHelpers.h"
#include "Buffer.h"
#include "BufferView.h"
#include "Image.h"
#include "ImageView.h"
#include "Framebuffer.h"
#include "Pipeline.h"
#include "DescriptorSetLayout.h"
#include "RenderPassCache.h"
#include "CommandBuffer.h"
#include "CommandPool.h"
#include "Device.h"
#include "StreamEncoder.h"

namespace vez
{
    StreamEncoder::StreamEncoder(CommandBuffer* commandBuffer, uint64_t memoryStreamBlockSize)
        : m_commandBuffer(commandBuffer)
        , m_stream(memoryStreamBlockSize)
    {

    }

    StreamEncoder::~StreamEncoder()
    {
        // Free any transient resources.
        for (auto destroyCallback : m_transientResources)
            destroyCallback();
    }

    void StreamEncoder::Begin()
    {
        // Free any transient resources.
        for (auto destroyCallback : m_transientResources)
            destroyCallback();

        // Reset the read and write positions in the memory stream.
        m_stream.SeekP(0);
        m_stream.SeekG(0);

        // Clear all internal state.
        m_graphicsState.Reset();
        m_resourceBindings.Reset();
        m_pipelineBarriers.Clear();
        m_descriptorSetBindings.clear();
        m_renderPasses.clear();
        m_pipelineBindings.clear();
        m_transientResources.clear();
        m_boundDescriptorSetLayouts.clear();
        m_inRenderPass = false;
    }

    void StreamEncoder::End()
    {
        // Get the collection of all image acceses in the PipelineBarriers object, which explicitly defines each image's last layout.
        auto& imageAccesses = m_pipelineBarriers.GetImageAccesses();
        if (imageAccesses.size() > 0)
        {
            // Populate a new pipeline barrier structure with any required image layout transitions.
            PipelineBarrier pipelineBarrier = {};
            pipelineBarrier.streamPosition = m_stream.TellP();
            for (auto itr : imageAccesses)
            {
                // Extract the image object from the key.
                auto image = reinterpret_cast<Image*>(itr.first[0]);

                // Every access to the image within the command buffer is transitioned back to the default layout independently of other accesses.
                auto& accessList = itr.second;
                for (auto& entry : accessList)
                {
                    if (entry.layout == image->GetDefaultImageLayout())
                        continue;

                    VkImageMemoryBarrier barrier = {};
                    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    barrier.dstAccessMask = barrier.srcAccessMask;
                    barrier.oldLayout = entry.layout;
                    barrier.newLayout = image->GetDefaultImageLayout();
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.image = image->GetHandle();
                    barrier.subresourceRange.aspectMask = GetImageAspectFlags(image->GetCreateInfo().format);
                    barrier.subresourceRange.baseMipLevel = entry.subresourceRange.baseMipLevel;
                    barrier.subresourceRange.levelCount = entry.subresourceRange.levelCount;
                    barrier.subresourceRange.baseArrayLayer = entry.subresourceRange.baseArrayLayer;
                    barrier.subresourceRange.layerCount = entry.subresourceRange.layerCount;
                    pipelineBarrier.imageBarriers.push_back(barrier);
                    pipelineBarrier.srcStageMask |= entry.stageMask;
                }
            }

            // All final image transitions should block any proceeding commands until the top of the pipeline stage is reached.
            pipelineBarrier.dstStageMask = pipelineBarrier.srcStageMask;

            // If any image barriers were added, add pipeline barrier to stream encoder at current write position.
            if (pipelineBarrier.imageBarriers.size() > 0)
                m_pipelineBarriers.GetBarriers().push_back(pipelineBarrier);
        }

        // Remove last pipeline barrier if srcStageMask and dstStageMask were never set.
        auto& barriers = m_pipelineBarriers.GetBarriers();
        if (!barriers.empty())
        {
            if (barriers.back().srcStageMask == 0 && barriers.back().dstStageMask == 0)
                barriers.pop_back();
        }
    }

    void StreamEncoder::TransitionImageLayout(Image* pImage, const VezImageSubresourceRange* range, VkImageLayout layout, VkAccessFlags accessMask, VkPipelineStageFlags stageMask)
    {
        // Add image layout transition to PipelineBarriers object.
        m_pipelineBarriers.ImageAccess(m_stream.TellP(), pImage, range, layout, accessMask, stageMask);
    }

    void StreamEncoder::CmdBeginRenderPass(const VezRenderPassBeginInfo* pBeginInfo)
    {
        // Mark the beginning of the renderpass.
        m_inRenderPass = true;

        // Reset the subpass index in the GraphicsState object.
        m_graphicsState.SetSubpassIndex(0);

        // Get current stream write position.
        auto streamPosition = m_stream.TellP();

        // Create a new RenderPassDesc instance with a single subpass by default.
        RenderPassDesc renderPassDesc = { };
        renderPassDesc.pNext = pBeginInfo->pNext;
        renderPassDesc.streamPosition = streamPosition;
        renderPassDesc.framebuffer = reinterpret_cast<Framebuffer*>(pBeginInfo->framebuffer);
        renderPassDesc.attachments.resize(pBeginInfo->attachmentCount);
        renderPassDesc.clearValues.resize(pBeginInfo->attachmentCount);
        for (auto i = 0U; i < pBeginInfo->attachmentCount; ++i)
        {
            // Retrieve the imageView for the given attachment index.
            auto imageView = renderPassDesc.framebuffer->GetAttachment(i);

            // Copy application supplied information into VkAttachmentReference object.
            auto& attachment = renderPassDesc.attachments[i];
            attachment.flags = 0;
            attachment.format = imageView->GetFormat();
            attachment.samples = imageView->GetImage()->GetCreateInfo().samples;
            attachment.loadOp = pBeginInfo->pAttachments[i].loadOp;
            attachment.storeOp = pBeginInfo->pAttachments[i].storeOp;
            attachment.stencilLoadOp = pBeginInfo->pAttachments[i].loadOp;
            attachment.stencilStoreOp = pBeginInfo->pAttachments[i].storeOp;
            attachment.initialLayout = m_pipelineBarriers.GetImageLayout(imageView);

            // For now, reset final layout to initial layout so as to keep command stream state well defined.
            // Later this should be refactored so finalLayout is stored in the PipelineBarriers object.
            attachment.finalLayout = attachment.initialLayout;

            // Copy application supplied information into VkClearValue object.
            auto& clearValue = renderPassDesc.clearValues[i];
            if (IsDepthStencilFormat(attachment.format))
            {
                clearValue.depthStencil.depth = pBeginInfo->pAttachments[i].clearValue.depthStencil.depth;
                clearValue.depthStencil.stencil = pBeginInfo->pAttachments[i].clearValue.depthStencil.stencil;
            }
            else
            {
                clearValue.color.float32[0] = pBeginInfo->pAttachments[i].clearValue.color.float32[0];
                clearValue.color.float32[1] = pBeginInfo->pAttachments[i].clearValue.color.float32[1];
                clearValue.color.float32[2] = pBeginInfo->pAttachments[i].clearValue.color.float32[2];
                clearValue.color.float32[3] = pBeginInfo->pAttachments[i].clearValue.color.float32[3];
            }
        }

        // Pipeline stage and access flags will be populated at the end of the subpass (either via CmdNextSubpass or CmdEndRenderPass).
        SubpassDesc firstSubpass = {};
        firstSubpass.streamPosition = streamPosition;
        firstSubpass.dependency.dependencyFlags = 0;
        firstSubpass.dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        firstSubpass.dependency.dstSubpass = 0;
        renderPassDesc.subpasses.push_back(firstSubpass);
        m_renderPasses.push_back(renderPassDesc);
    }

    void StreamEncoder::CmdNextSubpass()
    {
        // End the previous subpass.
        EndSubpass();

        // Add a new subpass to the current render pass (if command stream is inside of one at the moment).
        if (m_inRenderPass)
        {
            auto& prevSubpassDesc = m_renderPasses.back().subpasses.back();

            SubpassDesc nextSubpassDesc = {};
            nextSubpassDesc.streamPosition = m_stream.TellP();
            nextSubpassDesc.dependency.dependencyFlags = 0;
            nextSubpassDesc.dependency.srcSubpass = static_cast<uint32_t>(m_renderPasses.back().subpasses.size() - 1);
            nextSubpassDesc.dependency.dstSubpass = nextSubpassDesc.dependency.srcSubpass + 1;
            nextSubpassDesc.dependency.srcStageMask = prevSubpassDesc.dependency.dstStageMask;
            nextSubpassDesc.dependency.srcAccessMask = prevSubpassDesc.dependency.dstAccessMask;
            nextSubpassDesc.dependency.dstStageMask = 0;
            nextSubpassDesc.dependency.dstAccessMask = 0;
            m_renderPasses.back().subpasses.push_back(nextSubpassDesc);
        }

        // Update subpass index in the GraphicsState object.
        m_graphicsState.SetSubpassIndex(m_graphicsState.GetSubpassIndex() + 1);

        // Encode the command to the memory stream.
        m_stream << NEXT_SUBPASS;
    }

    void StreamEncoder::CmdEndRenderPass()
    {
        // End the last subpass.
        EndSubpass();

        // If CmdBeginRenderPass was not previously called before this, exit.
        if (!m_inRenderPass)
            return;

        // Get the current render pass description object.
        auto& renderPassDesc = m_renderPasses.back();

        // Request a compatible render pass from the RenderPassCache.
        auto device = m_commandBuffer->GetPool()->GetDevice();
        auto renderPassCache = device->GetRenderPassCache();
        auto result = renderPassCache->CreateRenderPass(&renderPassDesc, &renderPassDesc.renderPass);
        if (result != VK_SUCCESS)
        {
            // TODO: Handle error.
        }

        // Add render pass to transient resource list.
        m_transientResources.push_back([renderPassCache, renderPass = renderPassDesc.renderPass]() -> void {
            renderPassCache->DestroyRenderPass(renderPass);
        });

        // Iterate back over subpasses to create Vulkan handles for each bound pipeline.
        for (auto i = 0U; i < renderPassDesc.subpasses.size(); ++i)
        {
            // Iterate over each pipeline that was bound within the subpass.
            auto& subpassDesc = renderPassDesc.subpasses[i];
            for (auto entry : subpassDesc.pipelineBindings)
            {
                // Create native pipeline handle and store in stream's pipeline bindings.
                auto handle = entry.pipeline->GetHandle(renderPassDesc.renderPass, &entry.state);
                m_pipelineBindings.push_back({ entry.streamPosition, handle, entry.pipeline->GetBindPoint(), entry.pipeline->GetPipelineLayout() });
            }
        }

        // Determine the start and end stream positions of the render pass.
        auto startStreamPos = renderPassDesc.streamPosition;
        auto endStreamPos = m_stream.TellP();

        // See if any pipeline barriers were inserted into the command stream.
        auto& barriers = m_pipelineBarriers.GetBarriers();
        if (barriers.size() > 0)
        {
            // Find the first pipeline barrier after the start of the render pass.
            auto it = barriers.begin();
            while (it != barriers.end() && it->streamPosition < startStreamPos)
                ++it;

#if 0
            // Erase all pipeline barriers that occur within the renderpass.
            while (barriersItr != barriers.end() && barriersItr->streamPosition <= endStreamPos)
                barriersItr = barriers.erase(barriersItr);
#else
            // Aggregate all pipeline barriers into a single monolithic one to be inserted at the beginning of the render pass.
            if (it != barriers.end())
            {
                PipelineBarrier newBarrier = {};
                newBarrier.streamPosition = renderPassDesc.streamPosition;

                // Iterate over all pipeline barriers that occurred within render pass and combine them.
                while (it != barriers.end() && it->streamPosition <= endStreamPos)
                {
                    newBarrier.srcStageMask |= it->srcStageMask;
                    newBarrier.dstStageMask |= it->dstStageMask;
                    newBarrier.bufferBarriers.insert(newBarrier.bufferBarriers.end(), it->bufferBarriers.begin(), it->bufferBarriers.end());
                    newBarrier.imageBarriers.insert(newBarrier.imageBarriers.end(), it->imageBarriers.begin(), it->imageBarriers.end());
                    it = barriers.erase(it);
                }

                barriers.insert(it, newBarrier);
            }
#endif
        }

        // For all attachments referenced in the render pass, add finalLayouts and accesses to PipelineBarriers.
        auto framebuffer = reinterpret_cast<Framebuffer*>(renderPassDesc.framebuffer);
        for (auto i = 0U; i < renderPassDesc.attachments.size(); ++i)
        {
            auto imageView = framebuffer->GetAttachment(i);
            const auto& subresourceRange = imageView->GetSubresourceRange();
            auto image = imageView->GetImage();
            auto& attachment = renderPassDesc.attachments[i];

            auto access = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            auto stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            if (IsDepthStencilFormat(imageView->GetFormat()))
            {
                access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                stage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            }

            m_pipelineBarriers.ImageAccess(endStreamPos + 1ULL, image, &subresourceRange, attachment.finalLayout, access, stage);
        }

        // Mark that render pass has ended.
        m_inRenderPass = false;

        // Reset bound framebuffer in GraphicsState.
        m_graphicsState.SetFramebuffer(nullptr);

        // Encode the command to the memory stream.
        m_stream << END_RENDER_PASS;
    }

    void StreamEncoder::CmdBindPipeline(Pipeline* pPipeline)
    {
        // Track which pipeline is bound during command buffer recording.
        // This call is not encoded in stream but inserted into vector of pipeline bindings when StreamEncoder::BindPipeline() is called.
        m_graphicsState.SetPipeline(pPipeline);
    }

    void StreamEncoder::CmdPushConstants(uint32_t offset, uint32_t size, const void* pValues)
    {
        // A pipeline must be bound when this is called.
        auto pipeline = m_graphicsState.GetPipeline();
        if (pipeline)
        {
            // Encode the command, along with the pipeline's layout and shader stages push constant is used in, to the memory stream.
            auto layout = pipeline->GetPipelineLayout();
            auto shaderStages = pipeline->GetPushConstantsRangeStages(offset, size);
            m_stream << PUSH_CONSTANTS << layout << shaderStages << offset << size;
            m_stream.Write(pValues, static_cast<uint64_t>(size));
        }
    }

    void StreamEncoder::CmdBindBuffer(Buffer* pBuffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t arrayElement)
    {
        // Track resource bindings.  Do not encode in stream.
        m_resourceBindings.BindBuffer(pBuffer, offset, range, set, binding, arrayElement);
    }

    void StreamEncoder::CmdBindBufferView(BufferView* pBufferView, uint32_t set, uint32_t binding, uint32_t arrayElement)
    {
        // Track resource bindings.  Do not encode in stream.
        m_resourceBindings.BindBufferView(pBufferView, set, binding, arrayElement);
    }

    void StreamEncoder::CmdBindImageView(ImageView* pImageView, VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement)
    {
        // Track resource bindings.  Do not encode in stream.
        m_resourceBindings.BindImageView(pImageView, sampler, set, binding, arrayElement);
    }

    void StreamEncoder::CmdBindSampler(VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement)
    {
        // Track resource bindings.  Do not encode in stream.
        m_resourceBindings.BindSampler(sampler, set, binding, arrayElement);
    }

    void StreamEncoder::CmdBindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, Buffer** ppBuffers, const VkDeviceSize* pOffsets)
    {
        // Add buffer accesses to PipelineBarriers.
        for (auto i = 0U; i < bindingCount; ++i)
        {
            auto size = ppBuffers[i]->GetCreateInfo().size;
            m_pipelineBarriers.BufferAccess(m_stream.TellP(), ppBuffers[i], pOffsets[i], size - pOffsets[i], VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);
        }

        // Encode the command to the memory stream.
        m_stream << BIND_VERTEX_BUFFERS << firstBinding << bindingCount;
        m_stream.Write(ppBuffers, sizeof(Buffer*) * bindingCount);
        m_stream.Write(pOffsets, sizeof(VkDeviceSize) * bindingCount);
    }

    void StreamEncoder::CmdBindIndexBuffer(Buffer* pBuffer, VkDeviceSize offset, VkIndexType indexType)
    {
        // Add buffer access to PipelineBarriers.
        auto size = reinterpret_cast<Buffer*>(pBuffer)->GetCreateInfo().size;
        m_pipelineBarriers.BufferAccess(m_stream.TellP(), pBuffer, offset, size - offset, VK_ACCESS_INDEX_READ_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);

        // Encode the command to the memory stream.
        m_stream << BIND_INDEX_BUFFER << pBuffer << offset << indexType;
    }

    void StreamEncoder::CmdSetVertexInputFormat(VertexInputFormat* pVertexInputFormat)
    {
        // Store in GraphicsState.  Do not encode in stream.
        m_graphicsState.SetVertexInputFormat(pVertexInputFormat);
    }

    void StreamEncoder::CmdSetViewportState(uint32_t viewportCount)
    {
        // Store in GraphicsState.  Do not encode in stream.
        m_graphicsState.SetViewportState(viewportCount);
    }

    void StreamEncoder::CmdSetInputAssemblyState(const VezInputAssemblyState* pStateInfo)
    {
        // Store in GraphicsState.  Do not encode in stream.
        m_graphicsState.SetInputAssemblyState(pStateInfo);
    }

    void StreamEncoder::CmdSetRasterizationState(const VezRasterizationState* pStateInfo)
    {
        // Store in GraphicsState.  Do not encode in stream.
        m_graphicsState.SetRasterizationState(pStateInfo);
    }

    void StreamEncoder::CmdSetMultisampleState(const VezMultisampleState* pStateInfo)
    {
        // Store in GraphicsState.  Do not encode in stream.
        m_graphicsState.SetMultisampleState(pStateInfo);
    }

    void StreamEncoder::CmdSetDepthStencilState(const VezDepthStencilState* pStateInfo)
    {
        // Store in GraphicsState.  Do not encode in stream.
        m_graphicsState.SetDepthStencilState(pStateInfo);
    }

    void StreamEncoder::CmdSetColorBlendState(const VezColorBlendState* pStateInfo)
    {
        // Store in GraphicsState.  Do not encode in stream.
        m_graphicsState.SetColorBlendState(pStateInfo);
    }

    void StreamEncoder::CmdSetViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports)
    {
        // Encode the command to the memory stream.
        m_stream << SET_VIEWPORT << firstViewport << viewportCount;
        m_stream.Write(pViewports, sizeof(VkViewport) * viewportCount);
    }

    void StreamEncoder::CmdSetScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors)
    {
        // Encode the command to the memory stream.
        m_stream << SET_SCISSOR << firstScissor << scissorCount;
        m_stream.Write(pScissors, sizeof(VkRect2D) * scissorCount);
    }

    void StreamEncoder::CmdSetLineWidth(float lineWidth)
    {
        // Encode the command to the memory stream.
        m_stream << SET_LINE_WIDTH << lineWidth;
    }

    void StreamEncoder::CmdSetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
    {
        // Encode the command to the memory stream.
        m_stream << SET_DEPTH_BIAS << depthBiasConstantFactor << depthBiasClamp << depthBiasSlopeFactor;
    }

    void StreamEncoder::CmdSetBlendConstants(const float blendConstants[4])
    {
        // Encode the command to the memory stream.
        m_stream << SET_BLEND_CONSTANTS;
        m_stream.Write(blendConstants, sizeof(float) * 4);
    }

    void StreamEncoder::CmdSetDepthBounds(float minDepthBounds, float maxDepthBounds)
    {
        // Encode the command to the memory stream.
        m_stream << SET_DEPTH_BOUNDS << minDepthBounds << maxDepthBounds;
    }

    void StreamEncoder::CmdSetStencilCompareMask(VkStencilFaceFlags faceMask, uint32_t compareMask)
    {
        // Encode the command to the memory stream.
        m_stream << SET_STENCIL_COMPARE_MASK << faceMask << compareMask;
    }

    void StreamEncoder::CmdSetStencilWriteMask(VkStencilFaceFlags faceMask, uint32_t writeMask)
    {
        // Encode the command to the memory stream.
        m_stream << SET_STENCIL_WRITE_MASK << faceMask << writeMask;
    }

    void StreamEncoder::CmdSetStencilReference(VkStencilFaceFlags faceMask, uint32_t reference)
    {
        // Encode the command to the memory stream.
        m_stream << SET_STENCIL_REFERENCE << faceMask << reference;
    }

    void StreamEncoder::CmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        // Bind a new pipeline if any graphics state has changed.
        BindPipeline();

        // Create any required pipeline barriers and descriptor sets.
        BindDescriptorSet();

        // Encode the command to the memory stream.
        m_stream << DRAW << vertexCount << instanceCount << firstVertex << firstInstance;
    }

    void StreamEncoder::CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {
        // Bind a new pipeline if any graphics state has changed.
        BindPipeline();

        // Create any required pipeline barriers and descriptor sets.
        BindDescriptorSet();

        // Encode the command to the memory stream.
        m_stream << DRAW_INDEXED << indexCount << instanceCount << firstIndex << vertexOffset << firstInstance;
    }

    void StreamEncoder::CmdDrawIndirect(Buffer* pBuffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
    {
        // Add buffer access to PipelineBarriers.
        auto size = reinterpret_cast<Buffer*>(pBuffer)->GetCreateInfo().size;
        m_pipelineBarriers.BufferAccess(m_stream.TellP(), pBuffer, offset, size - offset, VK_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

        // Bind a new pipeline if any graphics state has changed.
        BindPipeline();

        // Create any required pipeline barriers and descriptor sets.
        BindDescriptorSet();

        // Encode the command to the memory stream.
        m_stream << DRAW_INDIRECT << pBuffer << offset << drawCount << stride;
    }

    void StreamEncoder::CmdDrawIndexedIndirect(Buffer* pBuffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
    {
        // Add buffer access to PipelineBarriers.
        auto size = reinterpret_cast<Buffer*>(pBuffer)->GetCreateInfo().size;
        m_pipelineBarriers.BufferAccess(m_stream.TellP(), pBuffer, offset, size - offset, VK_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

        // Bind a new pipeline if any graphics state has changed.
        BindPipeline();

        // Create any required pipeline barriers and descriptor sets.
        BindDescriptorSet();

        // Encode the command to the memory stream.
        m_stream << DRAW_INDEXED_INDIRECT << pBuffer << offset << drawCount << stride;
    }

    void StreamEncoder::CmdDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        // Bind a new pipeline if any graphics state has changed.
        BindPipeline();

        // Create any required pipeilne barriers and descriptor sets.
        BindDescriptorSet();

        // Encode the command to the memory stream.
        m_stream << DISPATCH << groupCountX << groupCountY << groupCountZ;
    }

    void StreamEncoder::CmdDispatchIndirect(Buffer* pBuffer, VkDeviceSize offset)
    {
        // Add buffer access to PipelineBarriers.
        auto size = reinterpret_cast<Buffer*>(pBuffer)->GetCreateInfo().size;
        m_pipelineBarriers.BufferAccess(m_stream.TellP(), pBuffer, offset, size - offset, VK_ACCESS_INDIRECT_COMMAND_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        // Bind a new pipeline if any graphics state has changed.
        BindPipeline();

        // Create any required pipeline barriers and descriptor sets.
        BindDescriptorSet();

        // Encode the command to the memory stream.
        m_stream << DISPATCH_INDIRECT << pBuffer << offset;
    }

    void StreamEncoder::CmdCopyBuffer(Buffer* pSrcBuffer, Buffer* pDstBuffer, uint32_t regionCount, const VezBufferCopy* pRegions)
    {
        // Add buffer accesses to PipelineBarriers.
        for (auto i = 0U; i < regionCount; ++i)
        {
            m_pipelineBarriers.BufferAccess(m_stream.TellP(), pSrcBuffer, pRegions[i].srcOffset, pRegions[i].size, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
            m_pipelineBarriers.BufferAccess(m_stream.TellP(), pDstBuffer, pRegions[i].dstOffset, pRegions[i].size, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        }

        // Encode the command to the memory stream.
        m_stream << COPY_BUFFER << pSrcBuffer << pDstBuffer << regionCount;
        m_stream.Write(pRegions, sizeof(VezBufferCopy) * regionCount);
    }

    void StreamEncoder::CmdCopyImage(Image* pSrcImage, Image* pDstImage, uint32_t regionCount, const VezImageCopy* pRegions)
    {
        // If source and destination images are the same, layouts must be equal.
        VkImageLayout srcLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        VkImageLayout dstLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        if (pSrcImage == pDstImage)
            srcLayout = dstLayout = VK_IMAGE_LAYOUT_GENERAL;

        // Add image accesses to PipelineBarriers.
        for (auto i = 0U; i < regionCount; ++i)
        {
            // Source image access.
            VezImageSubresourceRange subresourceRange = {};
            subresourceRange.baseMipLevel = pRegions[i].srcSubresource.mipLevel;
            subresourceRange.levelCount = 1;
            subresourceRange.baseArrayLayer = pRegions[i].srcSubresource.baseArrayLayer;
            subresourceRange.layerCount = pRegions[i].srcSubresource.layerCount;
            m_pipelineBarriers.ImageAccess(m_stream.TellP(), pSrcImage, &subresourceRange, srcLayout, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            // Destination image access.
            subresourceRange.baseMipLevel = pRegions[i].dstSubresource.mipLevel;
            subresourceRange.baseArrayLayer = pRegions[i].dstSubresource.baseArrayLayer;
            subresourceRange.layerCount = pRegions[i].dstSubresource.layerCount;
            m_pipelineBarriers.ImageAccess(m_stream.TellP(), pDstImage, &subresourceRange, dstLayout, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        }

        // Encode the command to the memory stream.
        m_stream << COPY_IMAGE << pSrcImage << pDstImage << regionCount;
        m_stream.Write(pRegions, sizeof(VezImageCopy) * regionCount);
        m_stream << srcLayout << dstLayout;
    }

    void StreamEncoder::CmdBlitImage(Image* pSrcImage, Image* pDstImage, uint32_t regionCount, const VezImageBlit* pRegions, VkFilter filter)
    {
        // If source and destination images are the same, layouts must be equal.
        VkImageLayout srcLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        VkImageLayout dstLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        if (pSrcImage == pDstImage)
            srcLayout = dstLayout = VK_IMAGE_LAYOUT_GENERAL;
        
        // Add image accesses to PipelineBarriers.
        for (auto i = 0U; i < regionCount; ++i)
        {
            // Source image access.
            VezImageSubresourceRange subresourceRange = {};
            subresourceRange.baseMipLevel = pRegions[i].srcSubresource.mipLevel;
            subresourceRange.levelCount = 1;
            subresourceRange.baseArrayLayer = pRegions[i].srcSubresource.baseArrayLayer;
            subresourceRange.layerCount = pRegions[i].srcSubresource.layerCount;
            m_pipelineBarriers.ImageAccess(m_stream.TellP(), pSrcImage, &subresourceRange, srcLayout, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            // Destination image access.
            subresourceRange.baseMipLevel = pRegions[i].dstSubresource.mipLevel;
            subresourceRange.baseArrayLayer = pRegions[i].dstSubresource.baseArrayLayer;
            subresourceRange.layerCount = pRegions[i].dstSubresource.layerCount;
            m_pipelineBarriers.ImageAccess(m_stream.TellP(), pDstImage, &subresourceRange, dstLayout, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        }

        // Encode the command to the memory stream.
        m_stream << BLIT_IMAGE << pSrcImage << pDstImage << regionCount;
        m_stream.Write(pRegions, sizeof(VezImageBlit) * regionCount);
        m_stream << filter << srcLayout << dstLayout;
    }

    void StreamEncoder::CmdCopyBufferToImage(Buffer* pSrcBuffer, Image* pDstImage, uint32_t regionCount, const VezBufferImageCopy* pRegions)
    {
        // Add image and buffer accesses to PipelineBarriers.
        for (auto i = 0U; i < regionCount; ++i)
        {
            // Source buffer access.
            auto bufferRange = static_cast<VkDeviceSize>(pRegions[i].bufferImageHeight) * pRegions[i].bufferRowLength;
            if (bufferRange == 0)
                bufferRange = reinterpret_cast<Buffer*>(pSrcBuffer)->GetCreateInfo().size - pRegions[i].bufferOffset;
            m_pipelineBarriers.BufferAccess(m_stream.TellP(), pSrcBuffer, pRegions[i].bufferOffset, bufferRange, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            // Destination image access.
            VezImageSubresourceRange subresourceRange = {};
            subresourceRange.baseMipLevel = pRegions[i].imageSubresource.mipLevel;
            subresourceRange.levelCount = 1;
            subresourceRange.baseArrayLayer = pRegions[i].imageSubresource.baseArrayLayer;
            subresourceRange.layerCount = pRegions[i].imageSubresource.layerCount;
            m_pipelineBarriers.ImageAccess(m_stream.TellP(), pDstImage, &subresourceRange, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        }

        // Encode the command to the memory stream.
        m_stream << COPY_BUFFER_TO_IMAGE << pSrcBuffer << pDstImage << regionCount;
        m_stream.Write(pRegions, sizeof(VezBufferImageCopy) * regionCount);
    }

    void StreamEncoder::CmdCopyImageToBuffer(Image* pSrcImage, Buffer* pDstBuffer, uint32_t regionCount, const VezBufferImageCopy* pRegions)
    {
        // Add image and buffer accesses to PipelineBarriers.
        for (auto i = 0U; i < regionCount; ++i)
        {
            // Source image access.
            VezImageSubresourceRange subresourceRange = {};
            subresourceRange.baseMipLevel = pRegions[i].imageSubresource.mipLevel;
            subresourceRange.levelCount = 1;
            subresourceRange.baseArrayLayer = pRegions[i].imageSubresource.baseArrayLayer;
            subresourceRange.layerCount = pRegions[i].imageSubresource.layerCount;
            m_pipelineBarriers.ImageAccess(m_stream.TellP(), pSrcImage, &subresourceRange, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            // Destination buffer access.
            auto bufferRange = static_cast<VkDeviceSize>(pRegions[i].bufferImageHeight) * pRegions[i].bufferRowLength;
            if (bufferRange == 0)
                bufferRange = reinterpret_cast<Buffer*>(pDstBuffer)->GetCreateInfo().size - pRegions[i].bufferOffset;
            m_pipelineBarriers.BufferAccess(m_stream.TellP(), pDstBuffer, pRegions[i].bufferOffset, bufferRange, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        }

        // Encode the command to the memory stream.
        m_stream << COPY_IMAGE_TO_BUFFER << pSrcImage << pDstBuffer << regionCount;
        m_stream.Write(pRegions, sizeof(VezBufferImageCopy) * regionCount);
    }

    void StreamEncoder::CmdUpdateBuffer(Buffer* pDstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData)
    {
        // Add buffer access to PipelineBarriers.
        m_pipelineBarriers.BufferAccess(m_stream.TellP(), pDstBuffer, dstOffset, dataSize, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        // Encode the command to the memory stream.
        m_stream << UPDATE_BUFFER << pDstBuffer << dstOffset << dataSize;
        m_stream.Write(pData, dataSize);
    }

    void StreamEncoder::CmdFillBuffer(Buffer* pDstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data)
    {
        // Add buffer access to PipelineBarriers.
        m_pipelineBarriers.BufferAccess(m_stream.TellP(), pDstBuffer, dstOffset, size, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        // Encode the command to the memory stream.
        m_stream << FILL_BUFFER << pDstBuffer << dstOffset << size << data;
    }

    void StreamEncoder::CmdClearColorImage(Image* pImage, const VkClearColorValue* pColor, uint32_t rangeCount, const VezImageSubresourceRange* pRanges)
    {
        // Add image accesses to PipelineBarriers.
        for (auto i = 0U; i < rangeCount; ++i)
            m_pipelineBarriers.ImageAccess(m_stream.TellP(), pImage, &pRanges[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        // Encode the command to the memory stream.
        m_stream << CLEAR_COLOR_IMAGE << pImage << *pColor << rangeCount;
        m_stream.Write(pRanges, sizeof(VezImageSubresourceRange) * rangeCount);
    }

    void StreamEncoder::CmdClearDepthStencilImage(Image* pImage, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VezImageSubresourceRange* pRanges)
    {
        // Add image accesses to PipelineBarriers.
        for (auto i = 0U; i < rangeCount; ++i)
            m_pipelineBarriers.ImageAccess(m_stream.TellP(), pImage, &pRanges[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        // Encode the command to the memory stream.
        m_stream << CLEAR_DEPTH_STENCIL_IMAGE << pImage << *pDepthStencil << rangeCount;
        m_stream.Write(pRanges, sizeof(VezImageSubresourceRange) * rangeCount);
    }

    void StreamEncoder::CmdClearAttachments(uint32_t attachmentCount, const VezClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects)
    {
        // Encode the command to the memory stream.
        m_stream << CLEAR_ATTACHMENTS << attachmentCount;
        m_stream.Write(pAttachments, sizeof(VezClearAttachment) * attachmentCount);
        m_stream << rectCount;
        m_stream.Write(pRects, sizeof(VkClearRect) * rectCount);
    }

    void StreamEncoder::CmdResolveImage(Image* pSrcImage, Image* pDstImage, uint32_t regionCount, const VezImageResolve* pRegions)
    {
        // Add image accesses to PipelineBarriers.
        for (auto i = 0U; i < regionCount; ++i)
        {
            // Source image access.
            VezImageSubresourceRange subresourceRange = {};
            subresourceRange.baseMipLevel = pRegions[i].srcSubresource.mipLevel;
            subresourceRange.levelCount = 1;
            subresourceRange.baseArrayLayer = pRegions[i].srcSubresource.baseArrayLayer;
            subresourceRange.layerCount = pRegions[i].srcSubresource.layerCount;
            m_pipelineBarriers.ImageAccess(m_stream.TellP(), pSrcImage, &subresourceRange, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

            // Destination image access.
            subresourceRange.baseMipLevel = pRegions[i].dstSubresource.mipLevel;
            subresourceRange.baseArrayLayer = pRegions[i].dstSubresource.baseArrayLayer;
            subresourceRange.layerCount = pRegions[i].dstSubresource.layerCount;
            m_pipelineBarriers.ImageAccess(m_stream.TellP(), pDstImage, &subresourceRange, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        }

        // Encode the command to the memory stream.
        m_stream << RESOLVE_IMAGE << pSrcImage << pDstImage << regionCount;
        m_stream.Write(pRegions, sizeof(VezImageResolve) * regionCount);
    }

    void StreamEncoder::CmdSetEvent(VkEvent event, VkPipelineStageFlags stageMask)
    {
        // Encode the command to the memory stream.
        m_stream << SET_EVENT << event << stageMask;
    }

    void StreamEncoder::CmdResetEvent(VkEvent event, VkPipelineStageFlags stageMask)
    {
        // Encode the command to the memory stream.
        m_stream << RESET_EVENT << event << stageMask;
    }

    void StreamEncoder::BindDescriptorSet()
    {
        // A valid pipeline must be bound before descriptor sets can be updated.
        auto pipeline = m_graphicsState.GetPipeline();
        if (pipeline)
        {
            // If any of the pipeline's descriptor set layouts do not match those currently bound, then new descriptor sets must be bound for that set indices.
            std::unordered_set<uint32_t> setConflicts;
            const auto& pipelineBindings = pipeline->GetBindings();
            for (auto it : pipelineBindings)
            {
                // For given set index, if descriptor set layouts differ, store set index in conflicts map.
                auto set = it.first;
                if (m_boundDescriptorSetLayouts.find(set) != m_boundDescriptorSetLayouts.end())
                {
                    if (m_boundDescriptorSetLayouts.at(set) != pipeline->GetDescriptorSetLayout(set))
                        setConflicts.emplace(set);
                }
            }

            // Remove entries from boundDescriptorSetLayouts that do not exist in pipeline.
            auto it = m_boundDescriptorSetLayouts.begin();
            while (it != m_boundDescriptorSetLayouts.end())
            {
                if (!pipeline->GetDescriptorSetLayout(it->first))
                    it = m_boundDescriptorSetLayouts.erase(it);
                else
                    ++it;
            }

            // Evaluate whether a new descriptor set must be created and bound.
            if (m_resourceBindings.IsDirty() || !setConflicts.empty())
            {
                // Reset resource bindings dirty bit.
                m_resourceBindings.ClearDirtyBit();

                // Iterate over each set binding.
                for (auto setBindingsItr : m_resourceBindings.GetSetBindings())
                {
                    // Skip if no bindings having changes.
                    auto set = setBindingsItr.first;
                    auto& setBindings = setBindingsItr.second;
                    if (!setBindings.dirty && (setConflicts.find(set) == setConflicts.end()))
                        continue;

                    // Reset set binding dirty flag.
                    setBindings.dirty = false;

                    // Retrieve the descriptor set layout for the given set index.
                    // If set index is not used with bound pipeline, skip it.
                    auto descriptorSetLayout = pipeline->GetDescriptorSetLayout(set);
                    if (!descriptorSetLayout)
                        continue;

                    // Allocate a new descriptor set. (TODO! Log or report error if allocation fails)
                    auto descriptorSet = descriptorSetLayout->AllocateDescriptorSet();
                    if (!descriptorSet)
                        continue;

                    // Set descriptor set layout as active for given set index.
                    m_boundDescriptorSetLayouts[set] = descriptorSetLayout;

                    // Update all of the set's bindings.
                    std::vector<VkDescriptorBufferInfo> bufferInfos;
                    std::vector<VkDescriptorImageInfo> imageInfos;
                    std::vector<VkWriteDescriptorSet> descriptorWrites;
                    for (auto bindingItr : setBindings.bindings)
                    {
                        // Get layout binding for given binding index.
                        auto binding = bindingItr.first;
                        VkDescriptorSetLayoutBinding* layoutBinding = nullptr;
                        if (!descriptorSetLayout->GetLayoutBinding(binding, &layoutBinding))
                            continue;

                        // Determine binding's access and pipeline stage flags.
                        auto accessMask = pipeline->GetBindingAccessFlags(set, binding);
                        auto shaderStages = static_cast<VkShaderStageFlags>(layoutBinding->stageFlags);

                        VkPipelineStageFlags stageMask = 0;
                        if (shaderStages & VK_SHADER_STAGE_VERTEX_BIT) stageMask |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
                        if (shaderStages & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) stageMask |= VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
                        if (shaderStages & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) stageMask |= VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
                        if (shaderStages & VK_SHADER_STAGE_GEOMETRY_BIT) stageMask |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
                        if (shaderStages & VK_SHADER_STAGE_FRAGMENT_BIT) stageMask |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                        if (shaderStages & VK_SHADER_STAGE_COMPUTE_BIT) stageMask |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

                        // Iterate over all array elements.
                        auto& arrayElementBindings = bindingItr.second;
                        for (auto arrayElementItr : arrayElementBindings)
                        {
                            // Get the binding info.
                            auto arrayElement = arrayElementItr.first;
                            auto& bindingInfo = arrayElementItr.second;

                            // Fill in descriptor set write structure.
                            VkWriteDescriptorSet dsWrite = {};
                            dsWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                            dsWrite.dstSet = descriptorSet;
                            dsWrite.dstBinding = binding;
                            dsWrite.dstArrayElement = arrayElement;
                            dsWrite.descriptorType = layoutBinding->descriptorType;
                            dsWrite.descriptorCount = 1;

                            // Handle buffers.
                            if (bindingInfo.pBuffer)
                            {
                                VkDescriptorBufferInfo bufferInfo = {};
                                bufferInfo.buffer = bindingInfo.pBuffer->GetHandle();
                                bufferInfo.offset = bindingInfo.offset;
                                bufferInfo.range = bindingInfo.range;
                                bufferInfos.push_back(bufferInfo);
                                dsWrite.pBufferInfo = reinterpret_cast<const VkDescriptorBufferInfo*>(bufferInfos.size());
                                m_pipelineBarriers.BufferAccess(m_stream.TellP(), bindingInfo.pBuffer, bindingInfo.offset, bindingInfo.range, accessMask, stageMask);
                            }
                            // Handle buffer views.
                            else if (bindingInfo.pBufferView)
                            {
                                dsWrite.pTexelBufferView = &bindingInfo.pBufferView->GetHandle();
                                m_pipelineBarriers.BufferAccess(m_stream.TellP(), bindingInfo.pBuffer, bindingInfo.pBufferView->GetOffset(), bindingInfo.pBufferView->GetRange(), accessMask, stageMask);
                            }
                            // Handle images and samplers.
                            else if (bindingInfo.pImageView || bindingInfo.sampler != VK_NULL_HANDLE)
                            {
                                VkDescriptorImageInfo imageInfo = {};

                                if (bindingInfo.sampler != VK_NULL_HANDLE)
                                {
                                    imageInfo.sampler = reinterpret_cast<VkSampler>(bindingInfo.sampler);
                                }

                                if (bindingInfo.pImageView)
                                {
                                    imageInfo.imageView = bindingInfo.pImageView->GetHandle();
                                    imageInfo.imageLayout = m_pipelineBarriers.GetImageLayout(bindingInfo.pImageView);
                                    
                                    switch (dsWrite.descriptorType)
                                    {
                                    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                                    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                                        //imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                        //m_pipelineBarriers.ImageAccess(m_stream.TellP(), bindingInfo.pImageView->GetImage(), &bindingInfo.pImageView->GetSubresourceRange(), imageInfo.imageLayout, accessMask, stageMask);
                                        break;

                                    case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                                        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                        break;

                                    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                                        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                                        m_pipelineBarriers.ImageAccess(m_stream.TellP(), bindingInfo.pImageView->GetImage(), &bindingInfo.pImageView->GetSubresourceRange(), imageInfo.imageLayout, accessMask, stageMask);
                                        break;

                                    default:
                                        continue;
                                    }
                                }

                                imageInfos.push_back(imageInfo);
                                dsWrite.pImageInfo = reinterpret_cast<const VkDescriptorImageInfo*>(imageInfos.size());
                            }

                            // Add the descriptor set write to the list.
                            descriptorWrites.push_back(dsWrite);
                        }
                    }

                    // Iterate back over descriptorWrites array and fix pointer addresses.
                    for (auto& dswrite : descriptorWrites)
                    {
                        if (dswrite.pBufferInfo)
                            dswrite.pBufferInfo = &bufferInfos[reinterpret_cast<uint64_t>(dswrite.pBufferInfo) - 1];
                        else if (dswrite.pImageInfo)
                            dswrite.pImageInfo = &imageInfos[reinterpret_cast<uint64_t>(dswrite.pImageInfo) - 1];
                    }

                    // Update descriptor set.
                    auto device = m_commandBuffer->GetPool()->GetDevice()->GetHandle();
                    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

                    // Store descriptor set binding for current stream encoder position.
                    DescriptorSetBinding dsb = { m_stream.TellP(), pipeline->GetBindPoint(), pipeline->GetPipelineLayout(), set, descriptorSet };
                    m_descriptorSetBindings.push_back(dsb);

                    // Add descriptor set to transient resource list. 
                    m_transientResources.push_back([descriptorSetLayout, descriptorSet]() -> void {
                        descriptorSetLayout->FreeDescriptorSet(descriptorSet);
                    });
                }
            }
        }
    }

    void StreamEncoder::BindPipeline()
    {
        // Ensure a pipeline is bound and the graphics state is dirty.
        auto pipeline = m_graphicsState.GetPipeline();
        if (pipeline)
        {
            // Check pipeline bind point.
            if (pipeline->GetBindPoint() == VK_PIPELINE_BIND_POINT_GRAPHICS)
            {
                // No need to re-create if graphics state is not dirty.
                if (!m_graphicsState.IsDirty())
                    return;

                // Reset graphics state dirty bit.
                m_graphicsState.ClearDirty();

                // If stream is in the middle of a render pass, add a SubpassPipelineBinding entry.
                if (m_inRenderPass)
                {
                    // Add entry.
                    auto& subpass = m_renderPasses.back().subpasses.back();
                    subpass.pipelineBindings.push_back({ m_stream.TellP(), pipeline, m_graphicsState });

                    // Update subpass outputAttachments array based on which locations pipeline writes to.
                    // Outputs from fragment shader stage will exist in set 0.
                    auto set0Bindings = pipeline->GetBindings().find(0);
                    if (set0Bindings != pipeline->GetBindings().cend())
                    {
                        // Find all output resource entries.
                        for (auto& resource : set0Bindings->second)
                        {
                            if (resource.stages == VK_SHADER_STAGE_FRAGMENT_BIT && resource.resourceType == VEZ_PIPELINE_RESOURCE_TYPE_OUTPUT)
                            {
                                // Add output location to subpass's outputAttachments array.
                                subpass.outputAttachments.emplace(resource.location);

                                // Set stage and access masks appropriately based on attachment format.
                                // Check for existence of attachment index for the case of GLSL error or framebuffer with no attachments.
                                auto framebuffer = reinterpret_cast<Framebuffer*>(m_renderPasses.back().framebuffer);
                                if (resource.location < framebuffer->GetAttachmentCount())
                                {
                                    auto imageView = framebuffer->GetAttachment(resource.location);
                                    if (IsDepthStencilFormat(imageView->GetFormat()))
                                    {
                                        subpass.dependency.dstStageMask |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                                        subpass.dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                                    }
                                    else
                                    {
                                        subpass.dependency.dstStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                                        subpass.dependency.dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                                    }
                                }
                            }
                        }
                    }

                    // Iterate over all sets and bindings to find any input attachments.
                    auto sets = pipeline->GetBindings();
                    for (auto& itr : sets)
                    {
                        auto& bindings = itr.second;
                        for (auto& entry : bindings)
                        {
                            if (entry.resourceType == VEZ_PIPELINE_RESOURCE_TYPE_INPUT_ATTACHMENT)
                            {
                                // Add image access to resource bindings so the input attachment is bound to a descriptor set.
                                auto framebuffer = reinterpret_cast<Framebuffer*>(m_renderPasses.back().framebuffer);
                                auto imageView = framebuffer->GetAttachment(entry.inputAttachmentIndex);
                                m_resourceBindings.BindImageView(imageView, VK_NULL_HANDLE, entry.set, entry.binding, 0);

                                // Add input attachment index to current subpass.
                                m_renderPasses.back().subpasses.back().inputAttachments.emplace(entry.inputAttachmentIndex);
                            }
                        }
                    }
                }
            }
            // Else the bound pipeline must be a compute pipeline.
            else if (pipeline->GetBindPoint() == VK_PIPELINE_BIND_POINT_COMPUTE)
            {
                // Store pipeline binding for current stream encoder position.
                m_pipelineBindings.push_back({ m_stream.TellP(), pipeline->GetHandle(VK_NULL_HANDLE, nullptr), pipeline->GetBindPoint(), pipeline->GetPipelineLayout() });
            }
        }
    }

    void StreamEncoder::EndSubpass()
    {
        // Get the current subpass dependency object.
        auto& subpassDependency = m_renderPasses.back().subpasses.back().dependency;

        // Determine appropriate bits for pipeline stage and access masks.  Simply do a bitwise or with the last pipeline barrier.
        // Pipeline barrier should then be removed from list since they will not be inserted during a subpass.
        // If stage masks are 0 (meaning no dependencies were found during stream encoding, set them to default values).
        if (!m_pipelineBarriers.GetBarriers().empty())
        {
            const auto& barrier = m_pipelineBarriers.GetBarriers().back();
            subpassDependency.srcStageMask |= barrier.srcStageMask;
            subpassDependency.dstStageMask |= barrier.dstStageMask;

            for (auto& entry : barrier.bufferBarriers)
            {
                subpassDependency.srcAccessMask |= entry.srcAccessMask;
                subpassDependency.dstAccessMask |= entry.dstAccessMask;
            }

            for (auto& entry : barrier.imageBarriers)
            {
                subpassDependency.srcAccessMask |= entry.srcAccessMask;
                subpassDependency.dstAccessMask |= entry.dstAccessMask;
            }
        }

        if (subpassDependency.srcStageMask == 0)
            subpassDependency.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        if (subpassDependency.dstStageMask == 0)
            subpassDependency.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }    
}