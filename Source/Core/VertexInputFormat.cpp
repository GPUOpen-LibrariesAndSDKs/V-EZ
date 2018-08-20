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
#include "VertexInputFormat.h"

namespace vez
{
    VkResult VertexInputFormat::Create(const VezVertexInputFormatCreateInfo* pCreateInfo, VertexInputFormat** ppVertexInputFormat)
    {
        auto vertexInputFormat = new VertexInputFormat;

        for (auto i = 0U; i < pCreateInfo->vertexBindingDescriptionCount; ++i)
        {
            VkVertexInputBindingDescription binding;
            binding.binding = pCreateInfo->pVertexBindingDescriptions[i].binding;
            binding.stride = pCreateInfo->pVertexBindingDescriptions[i].stride;
            binding.inputRate = static_cast<VkVertexInputRate>(pCreateInfo->pVertexBindingDescriptions[i].inputRate);
            vertexInputFormat->m_bindings.push_back(binding);
        }

        for (auto i = 0U; i < pCreateInfo->vertexAttributeDescriptionCount; ++i)
        {
            VkVertexInputAttributeDescription attribute;
            attribute.location = pCreateInfo->pVertexAttributeDescriptions[i].location;
            attribute.binding = pCreateInfo->pVertexAttributeDescriptions[i].binding;
            attribute.offset = pCreateInfo->pVertexAttributeDescriptions[i].offset;
            attribute.format = static_cast<VkFormat>(pCreateInfo->pVertexAttributeDescriptions[i].format);
            vertexInputFormat->m_attributes.push_back(attribute);
        }

        *ppVertexInputFormat = vertexInputFormat;

        return VK_SUCCESS;
    }

    VkPipelineVertexInputStateCreateInfo VertexInputFormat::GetStateCreateInfo() const
    {
        VkPipelineVertexInputStateCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        createInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(m_bindings.size());
        createInfo.pVertexBindingDescriptions = m_bindings.data();
        createInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_attributes.size());
        createInfo.pVertexAttributeDescriptions = m_attributes.data();
        return createInfo;
    }    
}