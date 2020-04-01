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
#include "Utility/VkHelpers.h"
#include "Device.h"
#include "Swapchain.h"
#include "Queue.h"
#include "Framebuffer.h"
#include "Image.h"
#include "ImageView.h"
#include "Buffer.h"
#include "BufferView.h"
#include "Pipeline.h"
#include "DescriptorSetLayout.h"
#include "RenderPassCache.h"
#include "CommandPool.h"
#include "CommandBuffer.h"

namespace vez
{
    CommandBuffer::CommandBuffer(CommandPool* pool, VkCommandBuffer handle, uint64_t streamEncoderBlockSize)
        : m_pool(pool)
        , m_handle(handle)
        , m_streamEncoder(this, streamEncoderBlockSize)
    {

    }

    CommandBuffer::~CommandBuffer()
    {
        // Free object handle.
        m_pool->FreeCommandBuffers(1, &m_handle);
    }

    VkResult CommandBuffer::Begin(VkCommandBufferUsageFlags flags)
    {
        // Don't allow Begin to be called more than once before End is called.
        if (m_isRecording)
            return VK_NOT_READY;

        // Begin command recording.
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = static_cast<VkCommandBufferUsageFlags>(flags);
        auto result = vkBeginCommandBuffer(m_handle, &beginInfo);
        if (result != VK_SUCCESS)
            return result;

        // Mark CommandBuffer as recording and reset internal state.
        m_isRecording = true;

        // Begin stream encoder.
        m_streamEncoder.Begin();

        // Set default dynamic states if the command buffer was created on a graphics queue.
        auto queueFamilyIndex = m_pool->GetQueueFamilyIndex();
        if (m_pool->GetDevice()->GetQueue(queueFamilyIndex, 0)->GetFlags() & VK_QUEUE_GRAPHICS_BIT)
        {
            CmdSetLineWidth(1.0f);
            CmdSetDepthBias(0.0f, 1.0f, 1.0f);
            CmdSetBlendConstants((std::array<float, 4> {1.0f, 1.0f, 1.0f, 1.0f }).data());
            CmdSetDepthBounds(0.0f, 1.0f);
        }

        // Return success.
        return VK_SUCCESS;
    }

    VkResult CommandBuffer::End()
    {
        // Begin must be called first.
        if (!m_isRecording)
            return VK_NOT_READY;

        // Mark CommandBuffer as not recording.
        m_isRecording = false;

        // End stream encoder.
        m_streamEncoder.End();

        // Decode command stream.
        m_streamDecoder.Decode(*this, m_streamEncoder);

        // End command recording.
        return vkEndCommandBuffer(m_handle);
    }

    VkResult CommandBuffer::Reset()
    {
        if (m_handle != VK_NULL_HANDLE)
            return vkResetCommandBuffer(m_handle, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
        return VK_SUCCESS;
    }

    void CommandBuffer::CmdBeginRenderPass(const VezRenderPassBeginInfo* pBeginInfo)
    {
        m_streamEncoder.CmdBeginRenderPass(pBeginInfo);
    }

    void CommandBuffer::CmdNextSubpass()
    {
        m_streamEncoder.CmdNextSubpass();
    }

    void CommandBuffer::CmdEndRenderPass()
    {
        m_streamEncoder.CmdEndRenderPass();
    }

    void CommandBuffer::CmdBindPipeline(Pipeline* pPipeline)
    {
        m_streamEncoder.CmdBindPipeline(pPipeline);
    }

    void CommandBuffer::CmdPushConstants(uint32_t offset, uint32_t size, const void* pValues)
    {
        m_streamEncoder.CmdPushConstants(offset, size, pValues);
    }

    void CommandBuffer::CmdBindBuffer(Buffer* pBuffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t arrayElement)
    {
        m_streamEncoder.CmdBindBuffer(pBuffer, offset, range, set, binding, arrayElement);
    }

    void CommandBuffer::CmdBindBufferView(BufferView* pBufferView, uint32_t set, uint32_t binding, uint32_t arrayElement)
    {
        m_streamEncoder.CmdBindBufferView(pBufferView, set, binding, arrayElement);
    }

    void CommandBuffer::CmdBindImageView(ImageView* pImageView, VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement)
    {
        m_streamEncoder.CmdBindImageView(pImageView, sampler, set, binding, arrayElement);
    }

    void CommandBuffer::CmdBindSampler(VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement)
    {
        m_streamEncoder.CmdBindSampler(sampler, set, binding, arrayElement);
    }

    void CommandBuffer::CmdBindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, Buffer** ppBuffers, const VkDeviceSize* pOffsets)
    {
        m_streamEncoder.CmdBindVertexBuffers(firstBinding, bindingCount, ppBuffers, pOffsets);
    }

    void CommandBuffer::CmdBindIndexBuffer(Buffer* pBuffer, VkDeviceSize offset, VkIndexType indexType)
    {
        m_streamEncoder.CmdBindIndexBuffer(pBuffer, offset, indexType);
    }

    void CommandBuffer::CmdSetVertexInputFormat(VertexInputFormat* pFormat)
    {
        m_streamEncoder.CmdSetVertexInputFormat(pFormat);
    }

    void CommandBuffer::CmdSetViewportState(uint32_t viewportCount)
    {
        m_streamEncoder.CmdSetViewportState(viewportCount);
    }

    void CommandBuffer::CmdSetInputAssemblyState(const VezInputAssemblyState* pStateInfo)
    {
        m_streamEncoder.CmdSetInputAssemblyState(pStateInfo);
    }

    void CommandBuffer::CmdSetRasterizationState(const VezRasterizationState* pStateInfo)
    {
        m_streamEncoder.CmdSetRasterizationState(pStateInfo);
    }

    void CommandBuffer::CmdSetMultisampleState(const VezMultisampleState* pStateInfo)
    {
        m_streamEncoder.CmdSetMultisampleState(pStateInfo);
    }

    void CommandBuffer::CmdSetDepthStencilState(const VezDepthStencilState* pStateInfo)
    {
        m_streamEncoder.CmdSetDepthStencilState(pStateInfo);
    }

    void CommandBuffer::CmdSetColorBlendState(const VezColorBlendState* pStateInfo)
    {
        m_streamEncoder.CmdSetColorBlendState(pStateInfo);
    }

	void CommandBuffer::CmdSetTessellationState(const VezTessellationState* pStateInfo)
	{
		m_streamEncoder.CmdSetTessellationState(pStateInfo);
	}

    void CommandBuffer::CmdSetViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports)
    {
        m_streamEncoder.CmdSetViewport(firstViewport, viewportCount, pViewports);
    }

    void CommandBuffer::CmdSetScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors)
    {
        m_streamEncoder.CmdSetScissor(firstScissor, scissorCount, pScissors);
    }

    void CommandBuffer::CmdSetLineWidth(float lineWidth)
    {
        m_streamEncoder.CmdSetLineWidth(lineWidth);
    }

    void CommandBuffer::CmdSetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
    {
        m_streamEncoder.CmdSetDepthBias(depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
    }

    void CommandBuffer::CmdSetBlendConstants(const float blendConstants[4])
    {
        m_streamEncoder.CmdSetBlendConstants(blendConstants);
    }

    void CommandBuffer::CmdSetDepthBounds(float minDepthBounds, float maxDepthBounds)
    {
        m_streamEncoder.CmdSetDepthBounds(minDepthBounds, maxDepthBounds);
    }

    void CommandBuffer::CmdSetStencilCompareMask(VkStencilFaceFlags faceMask, uint32_t compareMask)
    {
        m_streamEncoder.CmdSetStencilCompareMask(faceMask, compareMask);
    }

    void CommandBuffer::CmdSetStencilWriteMask(VkStencilFaceFlags faceMask, uint32_t writeMask)
    {
        m_streamEncoder.CmdSetStencilWriteMask(faceMask, writeMask);
    }

    void CommandBuffer::CmdSetStencilReference(VkStencilFaceFlags faceMask, uint32_t reference)
    {
        m_streamEncoder.CmdSetStencilReference(faceMask, reference);
    }

    void CommandBuffer::CmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        m_streamEncoder.CmdDraw(vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void CommandBuffer::CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {
        m_streamEncoder.CmdDrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void CommandBuffer::CmdDrawIndirect(Buffer* pBuffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
    {
        m_streamEncoder.CmdDrawIndirect(pBuffer, offset, drawCount, stride);
    }

    void CommandBuffer::CmdDrawIndexedIndirect(Buffer* pBuffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
    {
        m_streamEncoder.CmdDrawIndexedIndirect(pBuffer, offset, drawCount, stride);
    }

    void CommandBuffer::CmdDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        m_streamEncoder.CmdDispatch(groupCountX, groupCountY, groupCountZ);
    }

    void CommandBuffer::CmdDispatchIndirect(Buffer* pBuffer, VkDeviceSize offset)
    {
        m_streamEncoder.CmdDispatchIndirect(pBuffer, offset);
    }

    void CommandBuffer::CmdCopyBuffer(Buffer* pSrcBuffer, Buffer* pDstBuffer, uint32_t regionCount, const VezBufferCopy* pRegions)
    {
        m_streamEncoder.CmdCopyBuffer(pSrcBuffer, pDstBuffer, regionCount, pRegions);
    }

    void CommandBuffer::CmdCopyImage(Image* pSrcImage, Image* pDstImage, uint32_t regionCount, const VezImageCopy* pRegions)
    {
        m_streamEncoder.CmdCopyImage(pSrcImage, pDstImage, regionCount, pRegions);
    }

    void CommandBuffer::CmdBlitImage(Image* pSrcImage, Image* pDstImage, uint32_t regionCount, const VezImageBlit* pRegions, VkFilter filter)
    {
        m_streamEncoder.CmdBlitImage(pSrcImage, pDstImage, regionCount, pRegions, filter);
    }

    void CommandBuffer::CmdCopyBufferToImage(Buffer* pSrcBuffer, Image* pDstImage, uint32_t regionCount, const VezBufferImageCopy* pRegions)
    {
        m_streamEncoder.CmdCopyBufferToImage(pSrcBuffer, pDstImage, regionCount, pRegions);
    }

    void CommandBuffer::CmdCopyImageToBuffer(Image* pSrcImage, Buffer* pDstBuffer, uint32_t regionCount, const VezBufferImageCopy* pRegions)
    {
        m_streamEncoder.CmdCopyImageToBuffer(pSrcImage, pDstBuffer, regionCount, pRegions);
    }

    void CommandBuffer::CmdUpdateBuffer(Buffer* pDstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData)
    {
        m_streamEncoder.CmdUpdateBuffer(pDstBuffer, dstOffset, dataSize, pData);
    }

    void CommandBuffer::CmdFillBuffer(Buffer* pDstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data)
    {
        m_streamEncoder.CmdFillBuffer(pDstBuffer, dstOffset, size, data);
    }

    void CommandBuffer::CmdClearColorImage(Image* pImage, const VkClearColorValue* pColor, uint32_t rangeCount, const VezImageSubresourceRange* pRanges)
    {
        m_streamEncoder.CmdClearColorImage(pImage, pColor, rangeCount, pRanges);
    }

    void CommandBuffer::CmdClearDepthStencilImage(Image* pImage, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VezImageSubresourceRange* pRanges)
    {
        m_streamEncoder.CmdClearDepthStencilImage(pImage, pDepthStencil, rangeCount, pRanges);
    }

    void CommandBuffer::CmdClearAttachments(uint32_t attachmentCount, const VezClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects)
    {
        m_streamEncoder.CmdClearAttachments(attachmentCount, pAttachments, rectCount, pRects);
    }

    void CommandBuffer::CmdResolveImage(Image* pSrcImage, Image* pDstImage, uint32_t regionCount, const VezImageResolve* pRegions)
    {
        m_streamEncoder.CmdResolveImage(pSrcImage, pDstImage, regionCount, pRegions);
    }

    void CommandBuffer::CmdSetEvent(VkEvent event, VkPipelineStageFlags stageMask)
    {
        m_streamEncoder.CmdSetEvent(event, stageMask);
    }

    void CommandBuffer::CmdResetEvent(VkEvent event, VkPipelineStageFlags stageMask)
    {
        m_streamEncoder.CmdResetEvent(event, stageMask);
    }
}