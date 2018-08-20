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
#include "VEZ.h"

namespace vez
{
    typedef std::vector<uint64_t> GraphicsStateHash;

    class Framebuffer;
    class RenderPass;
    class Pipeline;
    class VertexInputFormat;

    class GraphicsState
    {
    public:
        GraphicsState();

        void Reset();

        void SetViewportState(uint32_t viewportCount);
        void SetVertexInputFormat(const VertexInputFormat* pVertexInputFormat);
        void SetInputAssemblyState(const VezInputAssemblyState* pStateInfo);
        void SetRasterizationState(const VezRasterizationState* pStateInfo);
        void SetMultisampleState(const VezMultisampleState* pStateInfo);
        void SetColorBlendState(const VezColorBlendState* pStateInfo);
        void SetDepthStencilState(const VezDepthStencilState* pStateInfo);
        void SetSubpassIndex(uint32_t index);
        void SetFramebuffer(Framebuffer* pFramebuffer);
        void SetPipeline(Pipeline* pPipeline);

        uint32_t GetViewportState() const { return m_viewportCount; }
        const VertexInputFormat* GetVertexInputFormat() const { return m_vertexInputFormat; }
        const VezInputAssemblyState& GetInputAssemblyState() const { return m_inputAssemblyState; }
        const VezRasterizationState& GetRasterizationState() const { return m_rasterizationState; }
        const VezMultisampleState& GetMultiSampleState() const { return m_multisampleState; }
        const VezColorBlendState& GetColorBlendState() const { return m_colorBlendState; }
        const VezColorBlendAttachmentState* GetColorBlendAttachmentState() const { return &m_colorBlendAttachments[0]; }
        const VezDepthStencilState& GetDepthStencilState() const { return m_depthStencilState; };
        uint32_t GetSubpassIndex() const { return m_subpassIndex; }
        Framebuffer* GetFramebuffer() const { return m_framebuffer; }
        Pipeline* GetPipeline() const { return m_pipeline; }

        bool IsDirty() const { return m_dirty; }
        void ClearDirty() { m_dirty = false; }

        GraphicsStateHash GetHash() const;

    private:
        uint32_t m_viewportCount = 1;
        const VertexInputFormat* m_vertexInputFormat = nullptr;
        VezInputAssemblyState m_inputAssemblyState;
        VezRasterizationState m_rasterizationState;
        VezMultisampleState m_multisampleState;
        VezDepthStencilState m_depthStencilState;
        VezColorBlendState m_colorBlendState;
        VezColorBlendAttachmentState m_colorBlendAttachments[8];
        uint32_t m_subpassIndex = 0;
        Framebuffer* m_framebuffer = nullptr;
        Pipeline* m_pipeline = nullptr;
        bool m_dirty = false;
    };    
}