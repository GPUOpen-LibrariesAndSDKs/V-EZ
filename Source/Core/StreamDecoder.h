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
#include "Utility/MemoryStream.h"
#include "VEZ.h"

namespace vez
{
    class Framebuffer;
    class CommandBuffer;
    class StreamEncoder;

    class StreamDecoder
    {
    public:
        StreamDecoder();

        void Decode(CommandBuffer& commandBuffer, StreamEncoder& encoder);

    private:
        void CmdBeginRenderPass(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdNextSubpass(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdEndRenderPass(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdBindPipeline(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdPushConstants(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdBindBuffer(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdBindBufferView(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdBindImageView(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdBindSampler(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdBindVertexBuffers(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdBindIndexBuffer(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdSetVertexInputFormat(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdSetViewportState(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdSetInputAssemblyState(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdSetRasterizationState(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdSetMultisampleState(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdSetDepthStencilState(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdSetColorBlendState(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdSetViewport(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdSetScissor(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdSetLineWidth(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdSetDepthBias(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdSetBlendConstants(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdSetDepthBounds(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdSetStencilCompareMask(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdSetStencilWriteMask(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdSetStencilReference(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdDraw(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdDrawIndexed(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdDrawIndirect(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdDrawIndexedIndirect(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdDispatch(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdDispatchIndirect(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdCopyBuffer(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdCopyImage(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdBlitImage(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdCopyBufferToImage(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdCopyImageToBuffer(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdUpdateBuffer(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdFillBuffer(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdClearColorImage(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdClearDepthStencilImage(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdClearAttachments(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdResolveImage(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdSetEvent(CommandBuffer& commandBuffer, MemoryStream& stream);
        void CmdResetEvent(CommandBuffer& commandBuffer, MemoryStream& stream);

        typedef void(StreamDecoder::*EntryPoint)(CommandBuffer&, MemoryStream&);
        std::vector<EntryPoint> m_entryPoints;
        Framebuffer* m_framebuffer = nullptr;
    };
}