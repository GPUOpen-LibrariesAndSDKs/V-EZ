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
#include "Buffer.h"
#include "Image.h"
#include "ImageView.h"
#include "Framebuffer.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "RenderPassCache.h"
#include "StreamEncoder.h"
#include "StreamDecoder.h"

namespace vez
{
    StreamDecoder::StreamDecoder()
    {
        // Populate entry points array with each command's decode function.
        m_entryPoints.resize(COMMAND_ID_COUNT);
        m_entryPoints[BEGIN_RENDER_PASS] = &StreamDecoder::CmdBeginRenderPass;
        m_entryPoints[NEXT_SUBPASS] = &StreamDecoder::CmdNextSubpass;
        m_entryPoints[END_RENDER_PASS] = &StreamDecoder::CmdEndRenderPass;
        m_entryPoints[BIND_PIPELINE] = &StreamDecoder::CmdBindPipeline;
        m_entryPoints[PUSH_CONSTANTS] = &StreamDecoder::CmdPushConstants;
        m_entryPoints[BIND_BUFFER] = &StreamDecoder::CmdBindBuffer;
        m_entryPoints[BIND_BUFFER_VIEW] = &StreamDecoder::CmdBindBufferView;
        m_entryPoints[BIND_IMAGE_VIEW] = &StreamDecoder::CmdBindImageView;
        m_entryPoints[BIND_SAMPLER] = &StreamDecoder::CmdBindSampler;
        m_entryPoints[BIND_VERTEX_BUFFERS] = &StreamDecoder::CmdBindVertexBuffers;
        m_entryPoints[BIND_INDEX_BUFFER] = &StreamDecoder::CmdBindIndexBuffer;
        m_entryPoints[SET_VERTEX_INPUT_FORMAT] = &StreamDecoder::CmdSetVertexInputFormat;
        m_entryPoints[SET_VIEWPORT_STATE] = &StreamDecoder::CmdSetViewportState;
        m_entryPoints[SET_INPUT_ASSEMBLY_STATE] = &StreamDecoder::CmdSetInputAssemblyState;
        m_entryPoints[SET_RASTERIZATION_STATE] = &StreamDecoder::CmdSetRasterizationState;
        m_entryPoints[SET_MULTISAMPLE_STATE] = &StreamDecoder::CmdSetMultisampleState;
        m_entryPoints[SET_DEPTH_STENCIL_STATE] = &StreamDecoder::CmdSetDepthStencilState;
        m_entryPoints[SET_COLOR_BLEND_STATE] = &StreamDecoder::CmdSetColorBlendState;
        m_entryPoints[SET_VIEWPORT] = &StreamDecoder::CmdSetViewport;
        m_entryPoints[SET_SCISSOR] = &StreamDecoder::CmdSetScissor;
        m_entryPoints[SET_LINE_WIDTH] = &StreamDecoder::CmdSetLineWidth;
        m_entryPoints[SET_DEPTH_BIAS] = &StreamDecoder::CmdSetDepthBias;
        m_entryPoints[SET_BLEND_CONSTANTS] = &StreamDecoder::CmdSetBlendConstants;
        m_entryPoints[SET_DEPTH_BOUNDS] = &StreamDecoder::CmdSetDepthBounds;
        m_entryPoints[SET_STENCIL_COMPARE_MASK] = &StreamDecoder::CmdSetStencilCompareMask;
        m_entryPoints[SET_STENCIL_WRITE_MASK] = &StreamDecoder::CmdSetStencilWriteMask;
        m_entryPoints[SET_STENCIL_REFERENCE] = &StreamDecoder::CmdSetStencilReference;
        m_entryPoints[DRAW] = &StreamDecoder::CmdDraw;
        m_entryPoints[DRAW_INDEXED] = &StreamDecoder::CmdDrawIndexed;
        m_entryPoints[DRAW_INDIRECT] = &StreamDecoder::CmdDrawIndirect;
        m_entryPoints[DRAW_INDEXED_INDIRECT] = &StreamDecoder::CmdDrawIndexedIndirect;
        m_entryPoints[DISPATCH] = &StreamDecoder::CmdDispatch;
        m_entryPoints[DISPATCH_INDIRECT] = &StreamDecoder::CmdDispatchIndirect;
        m_entryPoints[COPY_BUFFER] = &StreamDecoder::CmdCopyBuffer;
        m_entryPoints[COPY_IMAGE] = &StreamDecoder::CmdCopyImage;
        m_entryPoints[BLIT_IMAGE] = &StreamDecoder::CmdBlitImage;
        m_entryPoints[COPY_BUFFER_TO_IMAGE] = &StreamDecoder::CmdCopyBufferToImage;
        m_entryPoints[COPY_IMAGE_TO_BUFFER] = &StreamDecoder::CmdCopyImageToBuffer;
        m_entryPoints[UPDATE_BUFFER] = &StreamDecoder::CmdUpdateBuffer;
        m_entryPoints[FILL_BUFFER] = &StreamDecoder::CmdFillBuffer;
        m_entryPoints[CLEAR_COLOR_IMAGE] = &StreamDecoder::CmdClearColorImage;
        m_entryPoints[CLEAR_DEPTH_STENCIL_IMAGE] = &StreamDecoder::CmdClearDepthStencilImage;
        m_entryPoints[CLEAR_ATTACHMENTS] = &StreamDecoder::CmdClearAttachments;
        m_entryPoints[RESOLVE_IMAGE] = &StreamDecoder::CmdResolveImage;
        m_entryPoints[SET_EVENT] = &StreamDecoder::CmdSetEvent;
        m_entryPoints[RESET_EVENT] = &StreamDecoder::CmdResetEvent;
    }

    void StreamDecoder::Decode(CommandBuffer& commandBuffer, StreamEncoder& encoder)
    {
        // Get the memory stream from the stream encoder.
        auto& stream = encoder.GetStream();

        // Get the list of pipeline barriers to be inserted at specific points.
        auto nextPipelineBarrier = encoder.GetPipelineBarriers().cbegin();

        // Get the list of render passes to ber inserted at specific points.
        auto nextRenderPass = encoder.GetRenderPassBindings().cbegin();

        // Get the list of descriptor set bindings to be inserted at specific points.
        auto nextDescriptorSetBinding = encoder.GetDescriptorSetBindings().cbegin();

        // Get the list of pipeline bindings to be inserted at specific points.
        auto nextPipelineBinding = encoder.GetPipelineBindings().cbegin();

        // Seek to the beginning of the stream for reading.
        stream.SeekG(0);

        // Read all of the stream's data and decode each command.
        while (true)
        {
            // Get current stream read position.
            auto streamPosition = stream.TellG();

            // Check to see if there are any pipeline barriers to be inserted at or before current stream position.
            while ((nextPipelineBarrier != encoder.GetPipelineBarriers().cend()) && (nextPipelineBarrier->streamPosition <= streamPosition))
            {
                // Insert Vulkan pipeline barrier.
                vkCmdPipelineBarrier(commandBuffer.GetHandle(), nextPipelineBarrier->srcStageMask, nextPipelineBarrier->dstStageMask, 0, 0, nullptr,
                    static_cast<uint32_t>(nextPipelineBarrier->bufferBarriers.size()), nextPipelineBarrier->bufferBarriers.data(),
                    static_cast<uint32_t>(nextPipelineBarrier->imageBarriers.size()), nextPipelineBarrier->imageBarriers.data());

                // Move to the next pipeline barrier in the list.
                ++nextPipelineBarrier;
            }

            // Check to see if there are any render passes to be inserted.
            if (nextRenderPass != encoder.GetRenderPassBindings().cend())
            {
                // Next render pass's stream position must be equal to the current read position.
                if (nextRenderPass->streamPosition == streamPosition)
                {
                    // Store internal reference to bound framebuffer.
                    m_framebuffer = reinterpret_cast<Framebuffer*>(nextRenderPass->framebuffer);

                    // Begin render pass.
                    VkRenderPassBeginInfo beginInfo = {};
                    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                    beginInfo.renderPass = nextRenderPass->renderPass->GetHandle();
                    beginInfo.framebuffer = m_framebuffer->GetHandle(nextRenderPass->renderPass);
                    beginInfo.renderArea.offset.x = 0;
                    beginInfo.renderArea.offset.y = 0;
                    beginInfo.renderArea.extent = reinterpret_cast<Framebuffer*>(nextRenderPass->framebuffer)->GetExtents();
                    beginInfo.clearValueCount = static_cast<uint32_t>(nextRenderPass->clearValues.size());
                    beginInfo.pClearValues = nextRenderPass->clearValues.data();
                    vkCmdBeginRenderPass(commandBuffer.GetHandle(), &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

                    // Move to the next render pass in the list.
                    ++nextRenderPass;
                }
            }

            // Check to see if there are any pipeline bindings to be inserted.
            if (nextPipelineBinding != encoder.GetPipelineBindings().cend())
            {
                // The next pipeline binding's stream position must be equal to the current read position.
                if (nextPipelineBinding->streamPosition == streamPosition)
                {
                    // Bind the pipeline.
                    vkCmdBindPipeline(commandBuffer.GetHandle(), nextPipelineBinding->bindPoint, nextPipelineBinding->pipeline);

                    // Move to the next pipelien binding in the list.
                    ++nextPipelineBinding;
                }
            }

            // Check to see if there are any descriptor set bindings to be inserted.
            if (nextDescriptorSetBinding != encoder.GetDescriptorSetBindings().cend())
            {
                // The next descriptor set binding's stream position must be equal to the current read position.
                while (nextDescriptorSetBinding->streamPosition == streamPosition)
                {
                    // Bind the descriptor set.
                    vkCmdBindDescriptorSets(commandBuffer.GetHandle(), nextDescriptorSetBinding->bindPoint, nextDescriptorSetBinding->pipelineLayout,
                        nextDescriptorSetBinding->setIndex, 1, &nextDescriptorSetBinding->descriptorSet, 0, nullptr);

                    // Move to the next descriptor set binding in the list.
                    if (++nextDescriptorSetBinding == encoder.GetDescriptorSetBindings().cend())
                        break;
                }
            }

            // Parse and execute the next command in the stream.
            CommandID cmd;
            stream >> cmd;
            if (stream.EndOfStream())
                break;

            (this->*m_entryPoints[cmd])(commandBuffer, stream);
        }
    }

    void StreamDecoder::CmdBeginRenderPass(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // This command is never encoded by StreamEncoder.
    }

    void StreamDecoder::CmdNextSubpass(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Call native Vulkan function.
        vkCmdNextSubpass(commandBuffer.GetHandle(), VK_SUBPASS_CONTENTS_INLINE);
    }

    void StreamDecoder::CmdEndRenderPass(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Call native Vulkan function.
        vkCmdEndRenderPass(commandBuffer.GetHandle());

        // Reset bound framebuffer.
        m_framebuffer = nullptr;
    }

    void StreamDecoder::CmdBindPipeline(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // This command is never encoded by StreamEncoder.
    }

    void StreamDecoder::CmdPushConstants(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        VkPipelineLayout layout;
        VkShaderStageFlags shaderStages;
        uint32_t offset, size;
        stream >> layout >> shaderStages >> offset >> size;
        auto pValues = reinterpret_cast<const void*>(stream.ReadPtr<const uint8_t>(size));

        // Call native Vulkan function.       
        vkCmdPushConstants(commandBuffer.GetHandle(), layout, shaderStages, offset, size, pValues);
    }

    void StreamDecoder::CmdBindBuffer(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // This command is never encoded by StreamEncoder.
    }

    void StreamDecoder::CmdBindBufferView(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // This command is never encoded by StreamEncoder.
    }

    void StreamDecoder::CmdBindImageView(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // This command is never encoded by StreamEncoder.
    }

    void StreamDecoder::CmdBindSampler(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // This command is never encoded by StreamEncoder.
    }

    void StreamDecoder::CmdBindVertexBuffers(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        uint32_t firstBinding, bindingCount;
        stream >> firstBinding >> bindingCount;
        auto ppBuffers = stream.ReadPtr<Buffer*>(bindingCount);
        auto pOffsets = stream.ReadPtr<const VkDeviceSize>(bindingCount);

        // Call native Vulkan function.
        std::vector<VkBuffer> buffers(bindingCount);
        for (auto i = 0U; i < bindingCount; ++i)
            buffers[i] = ppBuffers[i]->GetHandle();

        vkCmdBindVertexBuffers(commandBuffer.GetHandle(), firstBinding, bindingCount, buffers.data(), reinterpret_cast<const VkDeviceSize*>(pOffsets));
    }

    void StreamDecoder::CmdBindIndexBuffer(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        Buffer* pBuffer;
        VkDeviceSize offset;
        VkIndexType indexType;
        stream >> pBuffer >> offset >> indexType;

        // Call native Vulkan function.
        vkCmdBindIndexBuffer(commandBuffer.GetHandle(), pBuffer->GetHandle(), offset, static_cast<VkIndexType>(indexType));
    }

    void StreamDecoder::CmdSetVertexInputFormat(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // This command is never encoded by StreamEncoder.
    }

    void StreamDecoder::CmdSetViewportState(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // This command is never encoded by StreamEncoder.
    }

    void StreamDecoder::CmdSetInputAssemblyState(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // This command is never encoded by StreamEncoder.
    }

    void StreamDecoder::CmdSetRasterizationState(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // This command is never encoded by StreamEncoder.
    }

    void StreamDecoder::CmdSetMultisampleState(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // This command is never encoded by StreamEncoder.
    }

    void StreamDecoder::CmdSetDepthStencilState(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // This command is never encoded by StreamEncoder.
    }

    void StreamDecoder::CmdSetColorBlendState(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // This command is never encoded by StreamEncoder.
    }

    void StreamDecoder::CmdSetViewport(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        uint32_t firstViewport, viewportCount;
        stream >> firstViewport >> viewportCount;
        auto pViewports = stream.ReadPtr<const VkViewport>(viewportCount);

        // Call native Vulkan function.
        vkCmdSetViewport(commandBuffer.GetHandle(), firstViewport, viewportCount, reinterpret_cast<const VkViewport*>(pViewports));
    }

    void StreamDecoder::CmdSetScissor(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        uint32_t firstScissor, scissorCount;
        stream >> firstScissor >> scissorCount;
        auto pScissors = stream.ReadPtr<const VkRect2D>(scissorCount);

        // Call native Vulkan function.
        vkCmdSetScissor(commandBuffer.GetHandle(), firstScissor, scissorCount, reinterpret_cast<const VkRect2D*>(pScissors));
    }

    void StreamDecoder::CmdSetLineWidth(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        float lineWidth;
        stream >> lineWidth;

        // Call native Vulkan function.
        vkCmdSetLineWidth(commandBuffer.GetHandle(), lineWidth);
    }

    void StreamDecoder::CmdSetDepthBias(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        float depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor;
        stream >> depthBiasConstantFactor >> depthBiasClamp >> depthBiasSlopeFactor;

        // Call native Vulkan function.
        vkCmdSetDepthBias(commandBuffer.GetHandle(), depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
    }

    void StreamDecoder::CmdSetBlendConstants(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        auto blendConstants = stream.ReadPtr<float>(4);

        // Call native Vulkan function.
        vkCmdSetBlendConstants(commandBuffer.GetHandle(), blendConstants);
    }

    void StreamDecoder::CmdSetDepthBounds(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        float minDepthBounds, maxDepthBounds;
        stream >> minDepthBounds >> maxDepthBounds;

        // Call native Vulkan function.
        vkCmdSetDepthBounds(commandBuffer.GetHandle(), minDepthBounds, maxDepthBounds);
    }

    void StreamDecoder::CmdSetStencilCompareMask(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        VkStencilFaceFlags faceMask;
        uint32_t compareMask;
        stream >> faceMask >> compareMask;

        // Call native Vulkan function.
        vkCmdSetStencilCompareMask(commandBuffer.GetHandle(), static_cast<VkStencilFaceFlags>(faceMask), compareMask);
    }

    void StreamDecoder::CmdSetStencilWriteMask(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        VkStencilFaceFlags faceMask;
        uint32_t writeMask;
        stream >> faceMask >> writeMask;

        // Call native Vulkan function.
        vkCmdSetStencilWriteMask(commandBuffer.GetHandle(), static_cast<VkStencilFaceFlags>(faceMask), writeMask);
    }

    void StreamDecoder::CmdSetStencilReference(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        VkStencilFaceFlags faceMask;
        uint32_t reference;
        stream >> faceMask >> reference;

        // Call native Vulkan function.
        vkCmdSetStencilReference(commandBuffer.GetHandle(), static_cast<VkStencilFaceFlags>(faceMask), reference);
    }

    void StreamDecoder::CmdDraw(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        uint32_t vertexCount, instanceCount, firstVertex, firstInstance;
        stream >> vertexCount >> instanceCount >> firstVertex >> firstInstance;

        // Call native Vulkan function.
        vkCmdDraw(commandBuffer.GetHandle(), vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void StreamDecoder::CmdDrawIndexed(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        uint32_t indexCount, instanceCount, firstIndex, vertexOffset, firstInstance;
        stream >> indexCount >> instanceCount >> firstIndex >> vertexOffset >> firstInstance;

        auto streamPos = stream.TellG();

        // Call native Vulkan function.
        vkCmdDrawIndexed(commandBuffer.GetHandle(), indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void StreamDecoder::CmdDrawIndirect(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        Buffer* pBuffer;
        VkDeviceSize offset;
        uint32_t drawCount, stride;
        stream >> pBuffer >> offset >> drawCount >> stride;

        // Call native Vulkan function.
        vkCmdDrawIndirect(commandBuffer.GetHandle(), pBuffer->GetHandle(), offset, drawCount, stride);
    }

    void StreamDecoder::CmdDrawIndexedIndirect(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        Buffer* pBuffer;
        VkDeviceSize offset;
        uint32_t drawCount, stride;
        stream >> pBuffer >> offset >> drawCount >> stride;

        // Call native Vulkan function.
        vkCmdDrawIndexedIndirect(commandBuffer.GetHandle(), pBuffer->GetHandle(), offset, drawCount, stride);
    }

    void StreamDecoder::CmdDispatch(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        uint32_t groupCountX, groupCountY, groupCountZ;
        stream >> groupCountX >> groupCountY >> groupCountZ;

        // Call native Vulkan function.
        vkCmdDispatch(commandBuffer.GetHandle(), groupCountX, groupCountY, groupCountZ);
    }

    void StreamDecoder::CmdDispatchIndirect(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        Buffer* pBuffer;
        VkDeviceSize offset;
        stream >> pBuffer >> offset;

        // Call native Vulkan function.
        vkCmdDispatchIndirect(commandBuffer.GetHandle(), pBuffer->GetHandle(), offset);
    }

    void StreamDecoder::CmdCopyBuffer(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        Buffer* pSrcBuffer;
        Buffer* pDstBuffer;
        uint32_t regionCount;
        stream >> pSrcBuffer >> pDstBuffer >> regionCount;
        auto pRegions = stream.ReadPtr<const VezBufferCopy>(regionCount);

        // Call native Vulkan function.
        std::vector<VkBufferCopy> regions(regionCount);
        for (auto i = 0U; i < regionCount; ++i)
        {
            regions[i].srcOffset = pRegions[i].srcOffset;
            regions[i].dstOffset = pRegions[i].dstOffset;
            regions[i].size = pRegions[i].size;
        }

        vkCmdCopyBuffer(commandBuffer.GetHandle(), pSrcBuffer->GetHandle(), pDstBuffer->GetHandle(), regionCount, regions.data());
    }

    void StreamDecoder::CmdCopyImage(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        Image* pSrcImage;
        Image* pDstImage;
        uint32_t regionCount;
        stream >> pSrcImage >> pDstImage >> regionCount;
        auto pRegions = stream.ReadPtr<const VezImageCopy>(regionCount);

        VkImageLayout srcLayout, dstLayout;
        stream >> srcLayout >> dstLayout;

        // Call native Vulkan function.
        std::vector<VkImageCopy> regions(regionCount);
        for (auto i = 0U; i < regionCount; ++i)
        {
            VkImageCopy& region = regions[i];
            memcpy(&region.srcOffset, &pRegions[i].srcOffset, sizeof(VkOffset3D));
            memcpy(&region.dstOffset, &pRegions[i].dstOffset, sizeof(VkOffset3D));
            memcpy(&region.extent, &pRegions[i].extent, sizeof(VkExtent3D));
            region.srcSubresource.aspectMask = GetImageAspectFlags(pSrcImage->GetCreateInfo().format);
            region.srcSubresource.mipLevel = pRegions[i].srcSubresource.mipLevel;
            region.srcSubresource.baseArrayLayer = pRegions[i].srcSubresource.baseArrayLayer;
            region.srcSubresource.layerCount = pRegions[i].srcSubresource.layerCount;
            region.dstSubresource.aspectMask = GetImageAspectFlags(pDstImage->GetCreateInfo().format);
            region.dstSubresource.mipLevel = pRegions[i].dstSubresource.mipLevel;
            region.dstSubresource.baseArrayLayer = pRegions[i].dstSubresource.baseArrayLayer;
            region.dstSubresource.layerCount = pRegions[i].dstSubresource.layerCount;
        }

        vkCmdCopyImage(commandBuffer.GetHandle(), pSrcImage->GetHandle(), srcLayout, pDstImage->GetHandle(), dstLayout, regionCount, regions.data());
    }

    void StreamDecoder::CmdBlitImage(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        Image* pSrcImage;
        Image* pDstImage;
        uint32_t regionCount;
        VkFilter filter;
        stream >> pSrcImage >> pDstImage >> regionCount;
        auto pRegions = stream.ReadPtr<const VezImageBlit>(regionCount);
        stream >> filter;

        VkImageLayout srcLayout, dstLayout;
        stream >> srcLayout >> dstLayout;

        // Call native Vulkan function.
        std::vector<VkImageBlit> regions(regionCount);
        for (auto i = 0U; i < regionCount; ++i)
        {
            VkImageBlit& region = regions[i];
            memcpy(&region.srcOffsets[0], &pRegions[i].srcOffsets[0], sizeof(VkOffset3D));
            memcpy(&region.srcOffsets[1], &pRegions[i].srcOffsets[1], sizeof(VkOffset3D));
            memcpy(&region.dstOffsets[0], &pRegions[i].dstOffsets[0], sizeof(VkOffset3D));
            memcpy(&region.dstOffsets[1], &pRegions[i].dstOffsets[1], sizeof(VkOffset3D));
            region.srcSubresource.aspectMask = GetImageAspectFlags(pSrcImage->GetCreateInfo().format);
            region.srcSubresource.mipLevel = pRegions[i].srcSubresource.mipLevel;
            region.srcSubresource.baseArrayLayer = pRegions[i].srcSubresource.baseArrayLayer;
            region.srcSubresource.layerCount = pRegions[i].srcSubresource.layerCount;
            region.dstSubresource.aspectMask = GetImageAspectFlags(pDstImage->GetCreateInfo().format);
            region.dstSubresource.mipLevel = pRegions[i].dstSubresource.mipLevel;
            region.dstSubresource.baseArrayLayer = pRegions[i].dstSubresource.baseArrayLayer;
            region.dstSubresource.layerCount = pRegions[i].dstSubresource.layerCount;
        }

        vkCmdBlitImage(commandBuffer.GetHandle(), pSrcImage->GetHandle(), srcLayout, pDstImage->GetHandle(), dstLayout, regionCount, regions.data(), static_cast<VkFilter>(filter));
    }

    void StreamDecoder::CmdCopyBufferToImage(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        Buffer* pSrcBuffer;
        Image* pDstImage;
        uint32_t regionCount;
        stream >> pSrcBuffer >> pDstImage >> regionCount;
        auto pRegions = stream.ReadPtr<const VezBufferImageCopy>(regionCount);

        // Call the native Vulkan function.
        std::vector<VkBufferImageCopy> regions(regionCount);
        for (auto i = 0U; i < regionCount; ++i)
        {
            VkBufferImageCopy& region = regions[i];
            region.bufferOffset = pRegions[i].bufferOffset;
            region.bufferRowLength = pRegions[i].bufferRowLength;
            region.bufferImageHeight = pRegions[i].bufferImageHeight;
            memcpy(&region.imageOffset, &pRegions[i].imageOffset, sizeof(VkOffset3D));
            memcpy(&region.imageExtent, &pRegions[i].imageExtent, sizeof(VkExtent3D));
            region.imageSubresource.aspectMask = GetImageAspectFlags(pDstImage->GetCreateInfo().format);
            region.imageSubresource.mipLevel = pRegions[i].imageSubresource.mipLevel;
            region.imageSubresource.baseArrayLayer = pRegions[i].imageSubresource.baseArrayLayer;
            region.imageSubresource.layerCount = pRegions[i].imageSubresource.layerCount;
        }

        vkCmdCopyBufferToImage(commandBuffer.GetHandle(), pSrcBuffer->GetHandle(), pDstImage->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regionCount, regions.data());
    }

    void StreamDecoder::CmdCopyImageToBuffer(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        Image* pSrcImage;
        Buffer* pDstBuffer;
        uint32_t regionCount;
        stream >> pSrcImage >> pDstBuffer >> regionCount;
        auto pRegions = stream.ReadPtr<const VezBufferImageCopy>(regionCount);

        // Call the native Vulkan function.
        std::vector<VkBufferImageCopy> regions(regionCount);
        for (auto i = 0U; i < regionCount; ++i)
        {
            VkBufferImageCopy& region = regions[i];
            region.bufferOffset = pRegions[i].bufferOffset;
            region.bufferRowLength = pRegions[i].bufferRowLength;
            region.bufferImageHeight = pRegions[i].bufferImageHeight;
            memcpy(&region.imageOffset, &pRegions[i].imageOffset, sizeof(VkOffset3D));
            memcpy(&region.imageExtent, &pRegions[i].imageExtent, sizeof(VkExtent3D));
            region.imageSubresource.aspectMask = GetImageAspectFlags(pSrcImage->GetCreateInfo().format);
            region.imageSubresource.mipLevel = pRegions[i].imageSubresource.mipLevel;
            region.imageSubresource.baseArrayLayer = pRegions[i].imageSubresource.baseArrayLayer;
            region.imageSubresource.layerCount = pRegions[i].imageSubresource.layerCount;
        }

        vkCmdCopyImageToBuffer(commandBuffer.GetHandle(), pSrcImage->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, pDstBuffer->GetHandle(), regionCount, regions.data());
    }

    void StreamDecoder::CmdUpdateBuffer(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        Buffer* pDstBuffer;
        VkDeviceSize dstOffset, dataSize;
        stream >> pDstBuffer >> dstOffset >> dataSize;
        auto pData = reinterpret_cast<const void*>(stream.ReadPtr<const uint8_t>(dataSize));

        // Call the native Vulkan function.
        vkCmdUpdateBuffer(commandBuffer.GetHandle(), pDstBuffer->GetHandle(), dstOffset, dataSize, pData);
    }

    void StreamDecoder::CmdFillBuffer(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        Buffer* pDstBuffer;
        VkDeviceSize dstOffset, size;
        uint32_t data;
        stream >> pDstBuffer >> dstOffset >> size >> data;

        // Call the native Vulkan function.
        vkCmdFillBuffer(commandBuffer.GetHandle(), pDstBuffer->GetHandle(), dstOffset, size, data);
    }

    void StreamDecoder::CmdClearColorImage(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        Image* pImage;
        stream >> pImage;
        auto pColor = stream.ReadPtr<const VkClearColorValue>();
        uint32_t rangeCount;
        stream >> rangeCount;
        auto pRanges = stream.ReadPtr<const VezImageSubresourceRange>(rangeCount);

        // Call the native Vulkan function.
        std::vector<VkImageSubresourceRange> ranges(rangeCount);
        for (auto i = 0U; i < rangeCount; ++i)
        {
            VkImageSubresourceRange& range = ranges[i];
            range.aspectMask = GetImageAspectFlags(pImage->GetCreateInfo().format);
            range.baseMipLevel = pRanges[i].baseMipLevel;
            range.levelCount = pRanges[i].levelCount;
            range.baseArrayLayer = pRanges[i].baseArrayLayer;
            range.layerCount = pRanges[i].layerCount;
        }

        vkCmdClearColorImage(commandBuffer.GetHandle(), pImage->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, reinterpret_cast<const VkClearColorValue*>(pColor), rangeCount, ranges.data());
    }

    void StreamDecoder::CmdClearDepthStencilImage(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        Image* pImage;
        stream >> pImage;
        auto pDepthStencil = stream.ReadPtr<const VkClearDepthStencilValue>();
        uint32_t rangeCount;
        stream >> rangeCount;
        auto pRanges = stream.ReadPtr<const VezImageSubresourceRange>(rangeCount);

        // Call the native Vulkan function.
        std::vector<VkImageSubresourceRange> ranges(rangeCount);
        for (auto i = 0U; i < rangeCount; ++i)
        {
            VkImageSubresourceRange& range = ranges[i];
            range.aspectMask = GetImageAspectFlags(pImage->GetCreateInfo().format);
            range.baseMipLevel = pRanges[i].baseMipLevel;
            range.levelCount = pRanges[i].levelCount;
            range.baseArrayLayer = pRanges[i].baseArrayLayer;
            range.layerCount = pRanges[i].layerCount;
        }

        vkCmdClearDepthStencilImage(commandBuffer.GetHandle(), pImage->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, reinterpret_cast<const VkClearDepthStencilValue*>(pDepthStencil), rangeCount, ranges.data());

    }

    void StreamDecoder::CmdClearAttachments(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        uint32_t attachmentCount;
        stream >> attachmentCount;
        auto pAttachments = stream.ReadPtr<const VezClearAttachment>(attachmentCount);
        uint32_t rectCount;
        stream >> rectCount;
        auto pRects = stream.ReadPtr<const VkClearRect>(rectCount);

        // Call the native Vulkan function.
        std::vector<VkClearAttachment> attachments(attachmentCount);
        for (auto i = 0U; i < attachmentCount; ++i)
        {
            VkClearAttachment& attachment = attachments[i];
            attachment.aspectMask = GetImageAspectFlags(m_framebuffer->GetAttachment(i)->GetImage()->GetCreateInfo().format);
            attachment.colorAttachment = pAttachments[i].colorAttachment;
            memcpy(&attachment.clearValue, &pAttachments[i].clearValue, sizeof(VkClearValue));
        }

        vkCmdClearAttachments(commandBuffer.GetHandle(), attachmentCount, attachments.data(), rectCount, reinterpret_cast<const VkClearRect*>(pRects));
    }

    void StreamDecoder::CmdResolveImage(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        Image* pSrcImage;
        Image* pDstImage;
        uint32_t regionCount;
        stream >> pSrcImage >> pDstImage >> regionCount;
        auto pRegions = stream.ReadPtr<const VezImageResolve>();

        // Call the native Vulkan function.
        std::vector<VkImageResolve> regions(regionCount);
        for (auto i = 0U; i < regionCount; ++i)
        {
            VkImageResolve& region = regions[i];
            memcpy(&region.srcOffset, &pRegions[i].srcOffset, sizeof(VkOffset3D));
            memcpy(&region.dstOffset, &pRegions[i].dstOffset, sizeof(VkOffset3D));
            memcpy(&region.extent, &pRegions[i].extent, sizeof(VkExtent3D));
            region.srcSubresource.aspectMask = GetImageAspectFlags(pSrcImage->GetCreateInfo().format);
            region.srcSubresource.mipLevel = pRegions[i].srcSubresource.mipLevel;
            region.srcSubresource.baseArrayLayer = pRegions[i].srcSubresource.baseArrayLayer;
            region.srcSubresource.layerCount = pRegions[i].srcSubresource.layerCount;
            region.dstSubresource.aspectMask = GetImageAspectFlags(pDstImage->GetCreateInfo().format);
            region.dstSubresource.mipLevel = pRegions[i].dstSubresource.mipLevel;
            region.dstSubresource.baseArrayLayer = pRegions[i].dstSubresource.baseArrayLayer;
            region.dstSubresource.layerCount = pRegions[i].dstSubresource.layerCount;
        }

        vkCmdResolveImage(commandBuffer.GetHandle(), pSrcImage->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, pDstImage->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regionCount, regions.data());

    }

    void StreamDecoder::CmdSetEvent(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        VkEvent event;
        VkPipelineStageFlags stageMask;
        stream >> event >> stageMask;

        // Call the native Vulkan function.
        vkCmdSetEvent(commandBuffer.GetHandle(), event, stageMask);
    }

    void StreamDecoder::CmdResetEvent(CommandBuffer& commandBuffer, MemoryStream& stream)
    {
        // Decode command parameters.
        VkEvent event;
        VkPipelineStageFlags stageMask;
        stream >> event >> stageMask;

        // Call the native Vulkan function.
        vkCmdResetEvent(commandBuffer.GetHandle(), event, stageMask);
    }
}