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

#include <stdint.h>
#include <vector>
#include <list>
#include <set>
#include <functional>
#include <map>
#include "Utility/MemoryStream.h"
#include "VEZ.h"
#include "GraphicsState.h"
#include "ResourceBindings.h"
#include "PipelineBarriers.h"

namespace vez
{
    // Forward declarations.
    class DescriptorSetLayout;
    class CommandBuffer;
    class RenderPass;
    class BufferView;

    // List of all VulkanEZ commands which are encoded during command buffer recording.
    typedef enum CommandID
    {
        BEGIN_RENDER_PASS,
        NEXT_SUBPASS,
        END_RENDER_PASS,
        BIND_PIPELINE,
        PUSH_CONSTANTS,
        BIND_BUFFER,
        BIND_BUFFER_VIEW,
        BIND_IMAGE_VIEW,
        BIND_SAMPLER,
        BIND_VERTEX_BUFFERS,
        BIND_INDEX_BUFFER,
        SET_VERTEX_INPUT_FORMAT,
        SET_VIEWPORT_STATE,
        SET_INPUT_ASSEMBLY_STATE,
        SET_RASTERIZATION_STATE,
        SET_MULTISAMPLE_STATE,
        SET_DEPTH_STENCIL_STATE,
        SET_COLOR_BLEND_STATE,
        SET_VIEWPORT,
        SET_SCISSOR,
        SET_LINE_WIDTH,
        SET_DEPTH_BIAS,
        SET_BLEND_CONSTANTS,
        SET_DEPTH_BOUNDS,
        SET_STENCIL_COMPARE_MASK,
        SET_STENCIL_WRITE_MASK,
        SET_STENCIL_REFERENCE,
        DRAW,
        DRAW_INDEXED,
        DRAW_INDIRECT,
        DRAW_INDEXED_INDIRECT,
        DISPATCH,
        DISPATCH_INDIRECT,
        COPY_BUFFER,
        COPY_IMAGE,
        BLIT_IMAGE,
        COPY_BUFFER_TO_IMAGE,
        COPY_IMAGE_TO_BUFFER,
        UPDATE_BUFFER,
        FILL_BUFFER,
        CLEAR_COLOR_IMAGE,
        CLEAR_DEPTH_STENCIL_IMAGE,
        CLEAR_ATTACHMENTS,
        RESOLVE_IMAGE,
        SET_EVENT,
        RESET_EVENT,
        COMMAND_ID_COUNT,
    } CommandID;

    // Type declaration for transient resource destruction lambdas (render passes, descriptor sets).
    typedef std::vector<std::function<void()>> TransientResources;

    // Descriptor set binding structure to be inserted into the command stream during decoding.
    struct DescriptorSetBinding
    {
        uint64_t streamPosition;
        VkPipelineBindPoint bindPoint;
        VkPipelineLayout pipelineLayout;
        uint32_t setIndex;
        VkDescriptorSet descriptorSet;
    };

    // Pipeline bindings that occurred within a subpass.
    struct SubpassPipelineBinding
    {
        uint64_t streamPosition;
        Pipeline* pipeline;
        GraphicsState state;
    };

    // Subpass structure for bound renderpass.
    struct SubpassDesc
    {
        uint64_t streamPosition;
        std::set<uint32_t> inputAttachments;
        std::set<uint32_t> outputAttachments;
        std::list<SubpassPipelineBinding> pipelineBindings;
        VkSubpassDependency dependency;
    };

    // RenderPass binding to be inserted into the command stream during decoding.
    struct RenderPassDesc
    {
        const void* pNext;
        uint64_t streamPosition;
        Framebuffer* framebuffer;
        std::vector<VkAttachmentDescription> attachments;
        std::vector<VkClearValue> clearValues;
        std::vector<SubpassDesc> subpasses;
        RenderPass* renderPass;
    };

    // Pipeline bindings to be inserted into the command stream during decoding.
    struct PipelineBinding
    {
        uint64_t streamPosition;
        VkPipeline pipeline;
        VkPipelineBindPoint bindPoint;
        VkPipelineLayout pipelineLayout;
    };

    // Command buffer stream encoder class for serializing incoming calls to an in memory binary stream.
    // The StreamEncoder class is responsible for automatic pipeline barrier insertion determination and descriptor set
    // creation from resource bindings.
    class StreamEncoder
    {
    public:
        StreamEncoder(CommandBuffer* comandBuffer, uint64_t memoryStreamBlockSize);
        
        ~StreamEncoder();

        MemoryStream& GetStream() { return m_stream; }

        const std::list<PipelineBarrier>& GetPipelineBarriers() { return m_pipelineBarriers.GetBarriers(); }

        const std::vector<RenderPassDesc>& GetRenderPassBindings() const { return m_renderPasses; }

        const std::vector<DescriptorSetBinding>& GetDescriptorSetBindings() const { return m_descriptorSetBindings; }

        const std::vector<PipelineBinding>& GetPipelineBindings() const { return m_pipelineBindings; }

        void Begin();

        void End();

        void TransitionImageLayout(Image* pImage, const VezImageSubresourceRange* range, VkImageLayout layout, VkAccessFlags accessMask, VkPipelineStageFlags stageMask);

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
        void CmdSetVertexInputFormat(VertexInputFormat* pVertexInputFormat);
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
        void BindDescriptorSet();
        void BindPipeline();
        void EndSubpass();

        CommandBuffer* m_commandBuffer;
        MemoryStream m_stream;
        GraphicsState m_graphicsState;
        ResourceBindings m_resourceBindings;
        PipelineBarriers m_pipelineBarriers;
        std::vector<RenderPassDesc> m_renderPasses;
        std::vector<DescriptorSetBinding> m_descriptorSetBindings;
        std::vector<PipelineBinding> m_pipelineBindings;
        TransientResources m_transientResources;
        std::unordered_map<uint32_t, DescriptorSetLayout*> m_boundDescriptorSetLayouts;
        bool m_inRenderPass = false;
    };    
}