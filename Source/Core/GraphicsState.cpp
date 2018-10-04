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
#include <iostream>
#include <type_traits>
#include <cstring>
#include <cmath>
#include "GraphicsState.h"

namespace vez
{
    static VezInputAssemblyState defaultInputAssemblyState = {
        nullptr,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        false
    };

    static VezRasterizationState defaultRasterizationState = {
        nullptr,
        false,
        false,
        VK_POLYGON_MODE_FILL,
        VK_CULL_MODE_NONE,
        VK_FRONT_FACE_COUNTER_CLOCKWISE,
        false
    };

    static VezMultisampleState defaultMultisampleState = {
        nullptr,
        VK_SAMPLE_COUNT_1_BIT,
        false,
        1.0f,
        nullptr,
        false,
        false
    };

    static VezDepthStencilState defaultDepthStencilState = {
        nullptr,
        false,
        true,
        VK_COMPARE_OP_LESS_OR_EQUAL,
        false,
        false,
        {
            VK_STENCIL_OP_REPLACE,
            VK_STENCIL_OP_REPLACE,
            VK_STENCIL_OP_REPLACE,
            VK_COMPARE_OP_NEVER,
        },
        {
            VK_STENCIL_OP_REPLACE,
            VK_STENCIL_OP_REPLACE,
            VK_STENCIL_OP_REPLACE,
            VK_COMPARE_OP_NEVER,
        }
    };

    static VezColorBlendState defaultColorBlendState = {
        nullptr,
        false,
        VK_LOGIC_OP_SET,
        0,
        nullptr,
    };

    static VezColorBlendAttachmentState defaultColorBlendAttachmentState = {
        false,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_OP_ADD,
        VK_BLEND_FACTOR_ONE,
        VK_BLEND_FACTOR_ZERO,
        VK_BLEND_OP_ADD,
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    template <typename T>
    static bool operator != (const T& a, const T& b)
    {
        constexpr bool typeSupported = std::is_base_of<VezInputAssemblyState, T>::value |
            std::is_base_of<VezRasterizationState, T>::value |
            std::is_base_of<VezMultisampleState, T>::value |
            std::is_base_of<VezDepthStencilState, T>::value;
        static_assert(typeSupported, "Invalid type");
        return (memcmp(&a, &b, sizeof(T)) != 0);
    }

    GraphicsState::GraphicsState()
    {
        Reset();
    }

    void GraphicsState::Reset()
    {
        m_viewportCount = 1;
        m_vertexInputFormat = nullptr;

        memcpy(&m_inputAssemblyState, &defaultInputAssemblyState, sizeof(VezInputAssemblyState));
        memcpy(&m_rasterizationState, &defaultRasterizationState, sizeof(VezRasterizationState));
        memcpy(&m_multisampleState, &defaultMultisampleState, sizeof(VezMultisampleState));
        memcpy(&m_depthStencilState, &defaultDepthStencilState, sizeof(VezDepthStencilState));
        memcpy(&m_colorBlendState, &defaultColorBlendState, sizeof(VezColorBlendState));
        for (uint32_t i = 0U; i < 8U; ++i)
            memcpy(&m_colorBlendAttachments[i], &defaultColorBlendAttachmentState, sizeof(VkPipelineColorBlendAttachmentState));

        m_colorBlendState.attachmentCount = 0;
        m_colorBlendState.pAttachments = &m_colorBlendAttachments[0];

        m_subpassIndex = 0;
        m_framebuffer = nullptr;
        m_pipeline = nullptr;
        m_dirty = false;
    }

    void GraphicsState::SetViewportState(uint32_t viewportCount)
    {
        m_viewportCount = viewportCount;
    }

    void GraphicsState::SetVertexInputFormat(const VertexInputFormat* pVertexInputFormat)
    {
        if (m_vertexInputFormat != pVertexInputFormat)
        {
            m_vertexInputFormat = pVertexInputFormat;
            m_dirty = true;
        }
    }

    void GraphicsState::SetInputAssemblyState(const VezInputAssemblyState* pStateInfo)
    {
        if (!pStateInfo)
        {
            if (m_inputAssemblyState != defaultInputAssemblyState)
            {
                memcpy(&m_inputAssemblyState, &defaultInputAssemblyState, sizeof(VezInputAssemblyState));
                m_dirty = true;
            }
        }
        else if (m_inputAssemblyState != *pStateInfo)
        {
            memcpy(&m_inputAssemblyState, pStateInfo, sizeof(VezInputAssemblyState));
            m_dirty = true;
        }
    }

    void GraphicsState::SetRasterizationState(const VezRasterizationState* pStateInfo)
    {
        if (!pStateInfo)
        {
            if (m_rasterizationState != defaultRasterizationState)
            {
                memcpy(&m_rasterizationState, &defaultRasterizationState, sizeof(VezRasterizationState));
                m_dirty = true;
            }
        }
        else if (m_rasterizationState != *pStateInfo)
        {
            memcpy(&m_rasterizationState, pStateInfo, sizeof(VezRasterizationState));
            m_dirty = true;
        }
    }

    void GraphicsState::SetMultisampleState(const VezMultisampleState* pStateInfo)
    {
        if (!pStateInfo)
        {
            if (m_multisampleState != defaultMultisampleState)
            {
                memcpy(&m_multisampleState, &defaultMultisampleState, sizeof(VezMultisampleState));
                m_dirty = true;
            }
        }
        else if (m_multisampleState != *pStateInfo)
        {
            memcpy(&m_multisampleState, pStateInfo, sizeof(VezMultisampleState));
            m_dirty = true;
        }
    }

    void GraphicsState::SetColorBlendState(const VezColorBlendState* pStateInfo)
    {
        if (pStateInfo)
        {
            m_colorBlendState.logicOpEnable = pStateInfo->logicOpEnable;
            m_colorBlendState.logicOp = pStateInfo->logicOp;
            m_colorBlendState.attachmentCount = pStateInfo->attachmentCount;
            for (auto i = 0U; i < pStateInfo->attachmentCount; ++i)
                m_colorBlendAttachments[i] = pStateInfo->pAttachments[i];
        }
        else
        {
            memcpy(&m_colorBlendState, &defaultColorBlendState, sizeof(VezColorBlendState));
            m_colorBlendState.attachmentCount = 0;
            m_colorBlendAttachments[0] = defaultColorBlendAttachmentState;
        }

        m_dirty = true;
    }

    void GraphicsState::SetDepthStencilState(const VezDepthStencilState* pStateInfo)
    {
        if (!pStateInfo)
        {
            if (m_depthStencilState != defaultDepthStencilState)
            {
                memcpy(&m_depthStencilState, &defaultDepthStencilState, sizeof(VezDepthStencilState));
                m_dirty = true;
            }
        }
        else if (m_depthStencilState != *pStateInfo)
        {
            memcpy(&m_depthStencilState, pStateInfo, sizeof(VezDepthStencilState));
            m_dirty = true;
        }
    }

    void GraphicsState::SetSubpassIndex(uint32_t index)
    {
        if (m_subpassIndex != index)
        {
            m_subpassIndex = index;
            m_dirty = true;
        }
    }

    void GraphicsState::SetFramebuffer(Framebuffer* pFramebuffer)
    {
        m_framebuffer = pFramebuffer;
    }

    void GraphicsState::SetPipeline(Pipeline* pPipeline)
    {
        if (m_pipeline != pPipeline)
        {
            m_pipeline = pPipeline;
            m_dirty = true;
        }
    }

    GraphicsStateHash GraphicsState::GetHash() const
    {
        // Graphics state bit field declarations for all possible pipeline state.
        struct GraphicsStateBitField
        {
            uint64_t vertexInputFormat : 64;
            uint32_t subpassIndex : 8;
            uint32_t topology : 4;
            uint32_t primitiveRestartEnable : 1;
            uint32_t depthClampEnable : 1;
            uint32_t rasterizationDiscardEnable : 1;
            uint32_t polygonMode : 2;
            uint32_t cullMode : 2;
            uint32_t frontFace : 1;
            uint32_t depthBiasEnable : 1;
            uint32_t sampleShadingEnable : 1;
            uint32_t rasterizationSamples : 3;
            uint32_t minSampleShading : 3;
            uint64_t sampleMask : 64;
            uint32_t alphaToCoverageEnable : 1;
            uint32_t alphaToOneEnable : 1;
            uint32_t logicOpEnable : 1;
            uint32_t logicOp : 4;
            uint32_t attachmentCount : 4;
            uint32_t depthTestEnable : 1;
            uint32_t depthWriteEnable : 1;
            uint32_t depthCompareOp : 3;
            uint32_t depthBoundsTestEnable : 1;
            uint32_t stencilTestEnable : 1;
            uint32_t frontFailOp : 3;
            uint32_t frontPassOp : 3;
            uint32_t frontDepthFailOp : 3;
            uint32_t frontCompareOp : 3;
            uint32_t backFailOp : 3;
            uint32_t backPassOp : 3;
            uint32_t backDepthFailOp : 3;
            uint32_t backCompareOp : 3;
        };

        struct ColorBlendAttachmentBitField
        {
            uint32_t colorWriteMask : 4;
            uint32_t blendEnable : 1;
            uint32_t srcColorBlendFactor : 5;
            uint32_t dstColorBlendFactor : 5;
            uint32_t colorBlendOp : 3;
            uint32_t srcAlphaBlendFactor : 5;
            uint32_t dstAlphaBlendFactor : 5;
            uint32_t alphaBlendOp : 3;
        };

        // Pipeline graphics state hash is (8) 64-bit integers stored in a vector.
        // Entries 0:3 store the GraphicsStateBitField object.
        // Entries 4:7 store a maximum of (8) ColorBlendAttachmentBitField objects (each 32-bits).
        const uint32_t maxAttachments = 8;
        size_t hashLength = (sizeof(GraphicsStateBitField) + sizeof(ColorBlendAttachmentBitField) * maxAttachments) >> 3;
        GraphicsStateHash hash(hashLength, 0ULL);

        // Populate the GraphicsStateBitField.
        GraphicsStateBitField* gb = reinterpret_cast<GraphicsStateBitField*>(&hash[0]);
        gb->vertexInputFormat = reinterpret_cast<uint64_t>(m_vertexInputFormat);
        gb->subpassIndex = m_subpassIndex;
        gb->topology = static_cast<uint32_t>(m_inputAssemblyState.topology);
        gb->primitiveRestartEnable = m_inputAssemblyState.primitiveRestartEnable;
        gb->depthClampEnable = m_rasterizationState.depthClampEnable;
        gb->rasterizationDiscardEnable = m_rasterizationState.rasterizerDiscardEnable;
        gb->polygonMode = static_cast<uint32_t>(m_rasterizationState.polygonMode);
        gb->cullMode = static_cast<uint32_t>(m_rasterizationState.cullMode);
        gb->frontFace = static_cast<uint32_t>(m_rasterizationState.frontFace);
        gb->depthBiasEnable = m_rasterizationState.depthBiasEnable;
        gb->sampleShadingEnable = m_multisampleState.sampleShadingEnable;
        gb->rasterizationSamples = static_cast<uint32_t>(log(static_cast<float>(m_multisampleState.rasterizationSamples)) / log(2.0f));
        gb->minSampleShading = static_cast<uint32_t>(m_multisampleState.minSampleShading * gb->rasterizationSamples);

        gb->sampleMask = 0;
        if (m_multisampleState.pSampleMask)
        {
            gb->sampleMask |= static_cast<uint64_t>(m_multisampleState.pSampleMask[0]);
            if (m_multisampleState.rasterizationSamples == VK_SAMPLE_COUNT_64_BIT)
                gb->sampleMask |= (static_cast<uint64_t>(m_multisampleState.pSampleMask[1]) << 32);
        }

        gb->alphaToCoverageEnable = m_multisampleState.alphaToCoverageEnable;
        gb->alphaToOneEnable = m_multisampleState.alphaToOneEnable;
        gb->logicOpEnable = m_colorBlendState.logicOpEnable;
        gb->logicOp = m_colorBlendState.logicOp;
        gb->attachmentCount = m_colorBlendState.attachmentCount;
        gb->depthTestEnable = m_depthStencilState.depthTestEnable;
        gb->depthWriteEnable = m_depthStencilState.depthWriteEnable;
        gb->depthCompareOp = static_cast<uint32_t>(m_depthStencilState.depthCompareOp);
        gb->depthBoundsTestEnable = m_depthStencilState.depthBoundsTestEnable;
        gb->stencilTestEnable = m_depthStencilState.stencilTestEnable;
        gb->frontFailOp = static_cast<uint32_t>(m_depthStencilState.front.failOp);
        gb->frontPassOp = static_cast<uint32_t>(m_depthStencilState.front.passOp);
        gb->frontDepthFailOp = static_cast<uint32_t>(m_depthStencilState.front.depthFailOp);
        gb->frontCompareOp = static_cast<uint32_t>(m_depthStencilState.front.compareOp);
        gb->backFailOp = static_cast<uint32_t>(m_depthStencilState.back.failOp);
        gb->backPassOp = static_cast<uint32_t>(m_depthStencilState.back.passOp);
        gb->backDepthFailOp = static_cast<uint32_t>(m_depthStencilState.back.depthFailOp);
        gb->backCompareOp = static_cast<uint32_t>(m_depthStencilState.back.compareOp);

        // Add all VkColorBlendAttachments.
        ColorBlendAttachmentBitField* cb = reinterpret_cast<ColorBlendAttachmentBitField*>(&hash[sizeof(GraphicsStateBitField) >> 3]);
        for (auto i = 0U; i < m_colorBlendState.attachmentCount; ++i)
        {
            cb->colorWriteMask = static_cast<uint32_t>(m_colorBlendState.pAttachments[i].colorWriteMask);
            cb->blendEnable = m_colorBlendState.pAttachments[i].blendEnable;
            cb->srcColorBlendFactor = static_cast<uint32_t>(m_colorBlendState.pAttachments[i].srcColorBlendFactor);
            cb->dstColorBlendFactor = static_cast<uint32_t>(m_colorBlendState.pAttachments[i].dstColorBlendFactor);
            cb->colorBlendOp = static_cast<uint32_t>(m_colorBlendState.pAttachments[i].colorBlendOp);
            cb->srcAlphaBlendFactor = static_cast<uint32_t>(m_colorBlendState.pAttachments[i].srcAlphaBlendFactor);
            cb->dstAlphaBlendFactor = static_cast<uint32_t>(m_colorBlendState.pAttachments[i].dstAlphaBlendFactor);
            cb->alphaBlendOp = static_cast<uint32_t>(m_colorBlendState.pAttachments[i].alphaBlendOp);
            ++cb;
        }

        return hash;
    }    
}