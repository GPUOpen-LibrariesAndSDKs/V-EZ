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

#include <vector>
#include <unordered_map>
#include <tuple>
#include <functional>
#include "VEZ.h"
#include "GraphicsState.h"
#include "ResourceBindings.h"
#include "StreamEncoder.h"
#include "StreamDecoder.h"

namespace vez
{
    class CommandPool;
    class Pipeline;
    class VertexInputFormat;
    class BufferView;
    class ImageView;

    class CommandBuffer
    {
    public:
        CommandBuffer(CommandPool* pool, VkCommandBuffer handle, uint64_t streamEncoderBlockSize);

        ~CommandBuffer();

        CommandPool* GetPool() { return m_pool; }

        VkCommandBuffer GetHandle() const { return m_handle; }

        VkResult Begin(VkCommandBufferUsageFlags flags);
        VkResult End();
        VkResult Reset();
        void CmdBeginRenderPass(const VezRenderPassBeginInfo* pBeginInfo);
        void CmdNextSubpass();
        void CmdEndRenderPass();
        void CmdBindPipeline(Pipeline* pPipeline);
        void CmdPushConstants(uint32_t offset, uint32_t size, const void* pValues);
        void CmdBindBuffer(Buffer* pBuffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t arrayElement);
        void CmdBindBufferView(BufferView* pBufferView, uint32_t set, uint32_t binding, uint32_t arrayElement);
        void CmdBindImageView(ImageView* pImageView, VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement);
        void CmdBindSampler(VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement);
        void CmdBindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, Buffer** ppBuffers, const VkDeviceSize* pOffsets);
        void CmdBindIndexBuffer(Buffer* pBuffer, VkDeviceSize offset, VkIndexType indexType);
        void CmdSetVertexInputFormat(VertexInputFormat* pFormat);
        void CmdSetViewportState(uint32_t viewportCount);
        void CmdSetInputAssemblyState(const VezInputAssemblyState* pStateInfo);
        void CmdSetRasterizationState(const VezRasterizationState* pStateInfo);
        void CmdSetMultisampleState(const VezMultisampleState* pStateInfo);
        void CmdSetDepthStencilState(const VezDepthStencilState* pStateInfo);
        void CmdSetColorBlendState(const VezColorBlendState* pStateInfo);
        void CmdSetTessellationState(const VezTessellationState* pStateInfo);
        void CmdSetViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports);
        void CmdSetScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors);
        void CmdSetLineWidth(float lineWidth);
        void CmdSetDepthBias(float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor);
        void CmdSetBlendConstants(const float blendConstants[4]);
        void CmdSetDepthBounds(float minDepthBounds, float maxDepthBounds);
        void CmdSetStencilCompareMask(VkStencilFaceFlags faceMask, uint32_t compareMask);
        void CmdSetStencilWriteMask(VkStencilFaceFlags faceMask, uint32_t writeMask);
        void CmdSetStencilReference(VkStencilFaceFlags faceMask, uint32_t reference);
        void CmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
        void CmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
        void CmdDrawIndirect(Buffer* pBuffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
        void CmdDrawIndexedIndirect(Buffer* pBuffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride);
        void CmdDispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
        void CmdDispatchIndirect(Buffer* pBuffer, VkDeviceSize offset);
        void CmdCopyBuffer(Buffer* pSrcBuffer, Buffer* pDstBuffer, uint32_t regionCount, const VezBufferCopy* pRegions);
        void CmdCopyImage(Image* pSrcImage, Image* pDstImage, uint32_t regionCount, const VezImageCopy* pRegions);
        void CmdBlitImage(Image* pSrcImage, Image* pDstImage, uint32_t regionCount, const VezImageBlit* pRegions, VkFilter filter);
        void CmdCopyBufferToImage(Buffer* pSrcBuffer, Image* pDstImage, uint32_t regionCount, const VezBufferImageCopy* pRegions);
        void CmdCopyImageToBuffer(Image* pSrcImage, Buffer* pDstBuffer, uint32_t regionCount, const VezBufferImageCopy* pRegions);
        void CmdUpdateBuffer(Buffer* pDstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData);
        void CmdFillBuffer(Buffer* pDstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data);
        void CmdClearColorImage(Image* pImage, const VkClearColorValue* pColor, uint32_t rangeCount, const VezImageSubresourceRange* pRanges);
        void CmdClearDepthStencilImage(Image* pImage, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VezImageSubresourceRange* pRanges);
        void CmdClearAttachments(uint32_t attachmentCount, const VezClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects);
        void CmdResolveImage(Image* pSrcImage, Image* pDstImage, uint32_t regionCount, const VezImageResolve* pRegions);
        void CmdSetEvent(VkEvent event, VkPipelineStageFlags stageMask);
        void CmdResetEvent(VkEvent event, VkPipelineStageFlags stageMask);

    private:
        CommandPool* m_pool;
        VkCommandBuffer m_handle = VK_NULL_HANDLE;
        StreamEncoder m_streamEncoder;
        StreamDecoder m_streamDecoder;
        bool m_isRecording = false;
        bool m_submittedToQueue = false;
    };  
}