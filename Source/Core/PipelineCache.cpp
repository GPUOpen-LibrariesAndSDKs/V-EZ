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
#include <algorithm>
#include <cstring>
#include "Utility/VkHelpers.h"
#include "Utility/ObjectLookup.h"
#include "Device.h"
#include "GraphicsState.h"
#include "VertexInputFormat.h"
#include "ShaderModule.h"
#include "Pipeline.h"
#include "Image.h"
#include "ImageView.h"
#include "Framebuffer.h"
#include "RenderPassCache.h"
#include "PipelineCache.h"

namespace vez
{
    PipelineCache::PipelineCache(Device* device)
        : m_device(device)
    {
        // Create Vulkan pipeline cache.
        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        auto result = vkCreatePipelineCache(m_device->GetHandle(), &pipelineCacheCreateInfo, nullptr, &m_vulkanPipelineCache);
        if (result != VK_SUCCESS)
        {
            // If this fails it's not catastrophic and shouldn't prevent V-EZ from continuing.
        }
    }

    PipelineCache::~PipelineCache()
    {
        // Destroy any remaining pipelines in cache.
        for (auto it : m_allPipelinesCache)
            vkDestroyPipeline(m_device->GetHandle(), it.second, nullptr);

        // Destroy vulkan pipeline cache.
        if (m_vulkanPipelineCache != VK_NULL_HANDLE)
            vkDestroyPipelineCache(m_device->GetHandle(), m_vulkanPipelineCache, nullptr);
    }

    VkResult PipelineCache::GetHandle(const Pipeline* pipeline, const RenderPass* pRenderPass, const GraphicsState* pState, VkPipeline* pHandle)
    {
        // Get the hash for the pipeline (compute pipeline hashes are only a single 64-bit entry with the memory address of the Pipeline class object).
        GraphicsStateHash hash;
        if (pipeline->GetBindPoint() == VK_PIPELINE_BIND_POINT_GRAPHICS)
            hash = GetPipelinePermutationHash(pipeline, pRenderPass, pState);
        else
            hash = { reinterpret_cast<uint64_t>(pipeline) };

        // Lock access to cache.
        m_spinLock.Lock();

        // Find the hash in the cache.
        auto it = m_allPipelinesCache.find(hash);
        if (it != m_allPipelinesCache.end())
        {
            *pHandle = it->second;
            m_spinLock.Unlock();
            return VK_SUCCESS;
        }
        else
        {
            // Unlock access to cache while pipeline is created.
            m_spinLock.Unlock();

            // Create a new pipeline.  For graphics, a new pipeline for the given state permutation is created.  For compute, there's always a single instance.
            VkResult result;
            if (pipeline->GetBindPoint() == VK_PIPELINE_BIND_POINT_GRAPHICS)
                result = CreateGraphicsPipeline(pipeline, pRenderPass, pState, pHandle);
            else
                result = CreateComputePipeline(pipeline, pHandle);

            // Lock access to cache.
            m_spinLock.Lock();

            // Add the pipeline object handle to the cache if creation was successful.
            if (result == VK_SUCCESS)
                m_allPipelinesCache.emplace(std::move(hash), *pHandle);

            // Release access to cache.
            m_spinLock.Unlock();

            // Return the result.
            return result;
        }
    }

    VkResult PipelineCache::CreateGraphicsPipeline(const Pipeline* pipeline, const RenderPass* pRenderPass, const GraphicsState* pState, VkPipeline* pHandle)
    {
        // Extract all specialization constant entries per stage.
        std::unordered_map<VkShaderStageFlagBits, VkSpecializationInfo> specializationInfos;
        for (auto i = 0U; i < pipeline->m_stages.size(); ++i)
        {
            auto shaderModule = ObjectLookup::GetImplShaderModule(pipeline->m_stages[i].module);
            auto stage = shaderModule->GetStage();
            auto it = pipeline->m_specializationInfo.find(stage);
            if (it != pipeline->m_specializationInfo.end())
            {
                VkSpecializationInfo specializationInfo = {};
                specializationInfo.mapEntryCount = static_cast<uint32_t>(it->second.mapEntries.size());
                specializationInfo.pMapEntries = it->second.mapEntries.data();
                specializationInfo.dataSize = static_cast<uint32_t>(it->second.data.size());
                specializationInfo.pData = it->second.data.data();
                specializationInfos.emplace(stage, specializationInfo);
            }
        }

        // Gather all of the pipeline stages.
        std::vector<VkPipelineShaderStageCreateInfo> stageCreateInfos(pipeline->m_stages.size());
        for (auto i = 0U; i < pipeline->m_stages.size(); ++i)
        {
            // Setup stage create info structure
            auto shaderModule = ObjectLookup::GetImplShaderModule(pipeline->m_stages[i].module);
            stageCreateInfos[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stageCreateInfos[i].stage = shaderModule->GetStage();
            stageCreateInfos[i].module = pipeline->m_stages[i].module;
            stageCreateInfos[i].pName = pipeline->m_entryPoints[i].c_str();

            // Get pointer to specialization info if it exists for shader stage.
            auto it = specializationInfos.find(stageCreateInfos[i].stage);
            if (it != specializationInfos.end())
                stageCreateInfos[i].pSpecializationInfo = &it->second;
        }

        // Get the current vertex input state.
        VkPipelineVertexInputStateCreateInfo vertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        std::vector<VkVertexInputAttributeDescription> defaultAttributes;
        std::vector<VkVertexInputBindingDescription> defaultBindings;
        if (pState->GetVertexInputFormat())
        {
            vertexInputState = pState->GetVertexInputFormat()->GetStateCreateInfo();
        }
        else
        {
            // Auto-generate a default vertex input format based on the vertex shader's resource inputs.
            std::vector<VezPipelineResource> inputs;
            for (auto& entry : pipeline->m_resources)
            {
                // Only consider input resources to vertex shader stage.
                auto& resource = entry.second;
                if (resource.stages & VK_SHADER_STAGE_VERTEX_BIT && resource.resourceType == VEZ_PIPELINE_RESOURCE_TYPE_INPUT)
                    inputs.push_back(resource);
            }

            // Sort the vertex inputs by location value.
            std::sort(inputs.begin(), inputs.end(), [](const VezPipelineResource& a, const VezPipelineResource& b) {
                return (a.location < b.location);
            });

            // Create the VkVertexInputAttributeDescription objects.
            uint32_t stride = 0;
            for (auto& input : inputs)
            {
                // Assume all attributes are interleaved in the same buffer bindings.
                VkVertexInputAttributeDescription attributeDescription = {};
                attributeDescription.binding = 0;
                attributeDescription.location = input.location;
                attributeDescription.offset = stride;

                uint32_t typeSize = 0;
                switch (input.baseType)
                {
                case VEZ_BASE_TYPE_CHAR:
                    typeSize = sizeof(char);
                    if (input.vecSize == 1) attributeDescription.format = VK_FORMAT_R8_SINT;
                    else if (input.vecSize == 2) attributeDescription.format = VK_FORMAT_R8G8_SINT;
                    else if (input.vecSize == 3) attributeDescription.format = VK_FORMAT_R8G8B8_SINT;
                    else if (input.vecSize == 4) attributeDescription.format = VK_FORMAT_R8G8B8A8_SINT;
                    break;

                case VEZ_BASE_TYPE_INT:
                    typeSize = sizeof(int32_t);
                    if (input.vecSize == 1) attributeDescription.format = VK_FORMAT_R32_SINT;
                    else if (input.vecSize == 2) attributeDescription.format = VK_FORMAT_R32G32_SINT;
                    else if (input.vecSize == 3) attributeDescription.format = VK_FORMAT_R32G32B32_SINT;
                    else if (input.vecSize == 4) attributeDescription.format = VK_FORMAT_R32G32B32A32_SINT;
                    break;

                case VEZ_BASE_TYPE_UINT:
                    typeSize = sizeof(uint32_t);
                    if (input.vecSize == 1) attributeDescription.format = VK_FORMAT_R32_UINT;
                    else if (input.vecSize == 2) attributeDescription.format = VK_FORMAT_R32G32_UINT;
                    else if (input.vecSize == 3) attributeDescription.format = VK_FORMAT_R32G32B32_UINT;
                    else if (input.vecSize == 4) attributeDescription.format = VK_FORMAT_R32G32B32A32_UINT;
                    break;

                case VEZ_BASE_TYPE_UINT64:
                    typeSize = sizeof(uint64_t);
                    if (input.vecSize == 1) attributeDescription.format = VK_FORMAT_R64_UINT;
                    else if (input.vecSize == 2) attributeDescription.format = VK_FORMAT_R64G64_UINT;
                    else if (input.vecSize == 3) attributeDescription.format = VK_FORMAT_R64G64B64_UINT;
                    else if (input.vecSize == 4) attributeDescription.format = VK_FORMAT_R64G64B64A64_UINT;
                    break;

                case VEZ_BASE_TYPE_HALF:
                    typeSize = sizeof(uint16_t);
                    if (input.vecSize == 1) attributeDescription.format = VK_FORMAT_R16_SFLOAT;
                    else if (input.vecSize == 2) attributeDescription.format = VK_FORMAT_R16G16_SFLOAT;
                    else if (input.vecSize == 3) attributeDescription.format = VK_FORMAT_R16G16B16_SFLOAT;
                    else if (input.vecSize == 4) attributeDescription.format = VK_FORMAT_R16G16B16A16_SFLOAT;
                    break;

                case VEZ_BASE_TYPE_FLOAT:
                    typeSize = sizeof(float);
                    if (input.vecSize == 1) attributeDescription.format = VK_FORMAT_R32_SFLOAT;
                    else if (input.vecSize == 2) attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
                    else if (input.vecSize == 3) attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
                    else if (input.vecSize == 4) attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
                    break;

                case VEZ_BASE_TYPE_DOUBLE:
                    typeSize = sizeof(double);
                    if (input.vecSize == 1) attributeDescription.format = VK_FORMAT_R64_SFLOAT;
                    else if (input.vecSize == 2) attributeDescription.format = VK_FORMAT_R64G64_SFLOAT;
                    else if (input.vecSize == 3) attributeDescription.format = VK_FORMAT_R64G64B64_SFLOAT;
                    else if (input.vecSize == 4) attributeDescription.format = VK_FORMAT_R64G64B64A64_SFLOAT;
                    break;
                }

                // Add the attribute description to the list.
                defaultAttributes.push_back(attributeDescription);

                // Increase the stride for the next vertex attribute.
                stride += typeSize * input.vecSize;
            }

            // Assume a single vertex buffer binding.
            if (defaultAttributes.size() > 0)
            {
                VkVertexInputBindingDescription bindingDescription = {};
                bindingDescription.binding = 0;
                bindingDescription.stride = stride;
                bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                defaultBindings.push_back(bindingDescription);
            }

            // Add the attribute and binding descriptions to the VkPipelineVertexInputStateCreateInfo object.
            vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(defaultAttributes.size());
            vertexInputState.pVertexAttributeDescriptions = defaultAttributes.data();
            vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(defaultBindings.size());
            vertexInputState.pVertexBindingDescriptions = defaultBindings.data();
        }

        // Get the input assembly state.
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
        inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyState.pNext = pState->GetInputAssemblyState().pNext;
        inputAssemblyState.topology = static_cast<VkPrimitiveTopology>(pState->GetInputAssemblyState().topology);
        inputAssemblyState.primitiveRestartEnable = pState->GetInputAssemblyState().primitiveRestartEnable;

        // Get the viewport state.
        std::vector<VkViewport> viewports(16);
        std::vector<VkRect2D> scissors(16);
        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = pState->GetViewportState();
        viewportState.pViewports = viewports.data();
        viewportState.scissorCount = pState->GetViewportState();
        viewportState.pScissors = scissors.data();

        // Get the rasterization state.
        VkPipelineRasterizationStateCreateInfo rasterizationState = {};
        rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationState.pNext = pState->GetRasterizationState().pNext;
        rasterizationState.depthClampEnable = pState->GetRasterizationState().depthClampEnable;
        rasterizationState.rasterizerDiscardEnable = pState->GetRasterizationState().rasterizerDiscardEnable;
        rasterizationState.polygonMode = static_cast<VkPolygonMode>(pState->GetRasterizationState().polygonMode);
        rasterizationState.lineWidth = 1.0f;
        rasterizationState.cullMode = static_cast<VkCullModeFlags>(pState->GetRasterizationState().cullMode);
        rasterizationState.frontFace = static_cast<VkFrontFace>(pState->GetRasterizationState().frontFace);
        rasterizationState.depthBiasEnable = pState->GetRasterizationState().depthBiasEnable;
        rasterizationState.depthBiasConstantFactor = 0.0f;
        rasterizationState.depthBiasClamp = 1.0f;
        rasterizationState.depthBiasSlopeFactor = 1.0f;
        
        // Get the multisample state.
        VkPipelineMultisampleStateCreateInfo multisampleState = {};
        multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleState.pNext = pState->GetMultiSampleState().pNext;
        multisampleState.sampleShadingEnable = pState->GetMultiSampleState().sampleShadingEnable;
        multisampleState.rasterizationSamples = static_cast<VkSampleCountFlagBits>(pState->GetMultiSampleState().rasterizationSamples);
        multisampleState.minSampleShading = pState->GetMultiSampleState().minSampleShading;
        multisampleState.pSampleMask = pState->GetMultiSampleState().pSampleMask;
        multisampleState.alphaToCoverageEnable = pState->GetMultiSampleState().alphaToCoverageEnable;
        multisampleState.alphaToOneEnable = pState->GetMultiSampleState().alphaToOneEnable;

        // Get the color blend state.
        VkPipelineColorBlendStateCreateInfo colorBlendState = {};
        colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendState.pNext = pState->GetColorBlendState().pNext;
        colorBlendState.logicOpEnable = pState->GetColorBlendState().logicOpEnable;
        colorBlendState.logicOp = static_cast<VkLogicOp>(pState->GetColorBlendState().logicOp);
        colorBlendState.attachmentCount = pState->GetColorBlendState().attachmentCount;
        colorBlendState.pAttachments = reinterpret_cast<const VkPipelineColorBlendAttachmentState*>(pState->GetColorBlendAttachmentState());
        if (colorBlendState.attachmentCount == 0)
            colorBlendState.attachmentCount = pRenderPass->GetColorAttachmentCount();

        colorBlendState.blendConstants[0] = 1.0f;
        colorBlendState.blendConstants[1] = 1.0f;
        colorBlendState.blendConstants[2] = 1.0f;
        colorBlendState.blendConstants[3] = 1.0f;

        // Get the depth stencil state.
        VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
        depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilState.pNext = pState->GetDepthStencilState().pNext;
        depthStencilState.depthTestEnable = pState->GetDepthStencilState().depthTestEnable;
        depthStencilState.depthWriteEnable = pState->GetDepthStencilState().depthWriteEnable;
        depthStencilState.depthCompareOp = static_cast<VkCompareOp>(pState->GetDepthStencilState().depthCompareOp);
        depthStencilState.depthBoundsTestEnable = pState->GetDepthStencilState().depthBoundsTestEnable;
        depthStencilState.stencilTestEnable = pState->GetDepthStencilState().stencilTestEnable;
        depthStencilState.front.failOp = static_cast<VkStencilOp>(pState->GetDepthStencilState().front.failOp);
        depthStencilState.front.passOp = static_cast<VkStencilOp>(pState->GetDepthStencilState().front.passOp);
        depthStencilState.front.depthFailOp = static_cast<VkStencilOp>(pState->GetDepthStencilState().front.depthFailOp);
        depthStencilState.front.compareOp = static_cast<VkCompareOp>(pState->GetDepthStencilState().front.compareOp);
        depthStencilState.front.compareMask = ~0U;
        depthStencilState.front.writeMask = ~0U;
        depthStencilState.front.reference = ~0U;
        depthStencilState.back.failOp = static_cast<VkStencilOp>(pState->GetDepthStencilState().back.failOp);
        depthStencilState.back.passOp = static_cast<VkStencilOp>(pState->GetDepthStencilState().back.passOp);
        depthStencilState.back.depthFailOp = static_cast<VkStencilOp>(pState->GetDepthStencilState().back.depthFailOp);
        depthStencilState.back.compareOp = static_cast<VkCompareOp>(pState->GetDepthStencilState().back.compareOp);
        depthStencilState.back.compareMask = ~0U;
        depthStencilState.back.writeMask = ~0U;
        depthStencilState.back.reference = ~0U;

        // Enable all dynamic state available.
        VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_LINE_WIDTH,
            VK_DYNAMIC_STATE_DEPTH_BIAS,
            VK_DYNAMIC_STATE_BLEND_CONSTANTS,
            VK_DYNAMIC_STATE_DEPTH_BOUNDS,
            VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
            VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
            VK_DYNAMIC_STATE_STENCIL_REFERENCE,
        };

        VkPipelineDynamicStateCreateInfo dynamicState = {};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
        dynamicState.pDynamicStates = dynamicStates;

        VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.pNext = pipeline->m_pNext;
        pipelineCreateInfo.stageCount = static_cast<uint32_t>(stageCreateInfos.size());
        pipelineCreateInfo.pStages = stageCreateInfos.data();
        pipelineCreateInfo.pVertexInputState = &vertexInputState;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
        pipelineCreateInfo.pViewportState = &viewportState;
        pipelineCreateInfo.pRasterizationState = &rasterizationState;
        pipelineCreateInfo.pMultisampleState = &multisampleState;
        pipelineCreateInfo.pDepthStencilState = &depthStencilState;
        pipelineCreateInfo.pColorBlendState = &colorBlendState;
        pipelineCreateInfo.pDynamicState = &dynamicState;
        pipelineCreateInfo.layout = pipeline->m_pipelineLayout;
        pipelineCreateInfo.renderPass = pRenderPass->GetHandle();
        pipelineCreateInfo.subpass = pState->GetSubpassIndex();

        // Create the Vulkan pipeline handle.
        auto result = vkCreateGraphicsPipelines(m_device->GetHandle(), m_vulkanPipelineCache, 1, &pipelineCreateInfo, nullptr, pHandle);
        return result;
    }

    VkResult PipelineCache::CreateComputePipeline(const Pipeline* pipeline, VkPipeline* pHandle)
    {
        // Create the pipeline handle.
        auto shaderModule = ObjectLookup::GetImplShaderModule(pipeline->m_stages[0].module);
        VkComputePipelineCreateInfo pipelineCreateInfo = {};
        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipelineCreateInfo.stage.stage = shaderModule->GetStage();
        pipelineCreateInfo.stage.module = shaderModule->GetHandle();
        pipelineCreateInfo.stage.pName = pipeline->m_entryPoints[0].c_str();
        pipelineCreateInfo.layout = pipeline->m_pipelineLayout;
        return vkCreateComputePipelines(m_device->GetHandle(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, pHandle);
    }

    PipelineCache::PipelinePermutationHash PipelineCache::GetPipelinePermutationHash(const Pipeline* pipeline, const RenderPass* pRenderPass, const GraphicsState* pState)
    {
        // If pipeline is compute only, the graphics state is not needed to compute the hash.
        if (pipeline->GetBindPoint() == VK_PIPELINE_BIND_POINT_COMPUTE)
        {
            PipelinePermutationHash singlePipelineInstanceHash = { reinterpret_cast<uint64_t>(pipeline) };
            return singlePipelineInstanceHash;
        }
        else
        {
            // The GraphicsState class generates it's own hash.  The PipelinePermutationHash adds another vector element to the beginning
            // containing the pointer address to the pipeline.
            auto graphicsStateHash = pState->GetHash();
            PipelinePermutationHash pipelinePermutationHash(2 + graphicsStateHash.size());
            pipelinePermutationHash[0] = reinterpret_cast<uint64_t>(pipeline);
            pipelinePermutationHash[1] = reinterpret_cast<uint64_t>(pRenderPass);
            memcpy(&pipelinePermutationHash[2], &graphicsStateHash[0], sizeof(uint64_t) * graphicsStateHash.size());
            return pipelinePermutationHash;
        }
    }    
}