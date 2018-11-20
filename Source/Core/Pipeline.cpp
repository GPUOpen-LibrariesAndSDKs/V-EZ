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
#include <algorithm>
#include <unordered_map>
#include "Utility/VkHelpers.h"
#include "Utility/ObjectLookup.h"
#include "Device.h"
#include "Swapchain.h"
#include "Framebuffer.h"
#include "VertexInputFormat.h"
#include "DescriptorSetLayout.h"
#include "DescriptorSetLayoutCache.h"
#include "PipelineCache.h"
#include "ShaderModule.h"
#include "ImageView.h"
#include "Image.h"
#include "Pipeline.h"

namespace vez
{
    VkResult Pipeline::Create(Device* pDevice, const VezGraphicsPipelineCreateInfo* pCreateInfo, Pipeline** ppPipeline)
    {
        // Get class objects for shader modules and validate there are no compute shader modules present.
        // Count the total number of stages with specialization constants and total number of map entries.
        std::vector<ShaderModule*> shaderModules(pCreateInfo->stageCount);
        for (auto i = 0U; i < pCreateInfo->stageCount; ++i)
        {
            shaderModules[i] = ObjectLookup::GetImplShaderModule(pCreateInfo->pStages[i].module);
            if (shaderModules[i]->GetStage() == VK_SHADER_STAGE_COMPUTE_BIT)
                return VK_INCOMPLETE;
        }

        
        // Create a new instance of the Pipeline class.
        auto pipeline = new Pipeline;
        pipeline->m_device = pDevice;
        pipeline->m_bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        // Copy the array of stage info.
        pipeline->m_stages.resize(pCreateInfo->stageCount);
        for (auto i = 0U; i < pCreateInfo->stageCount; ++i)
        {
            // Validate shader module was successfully created.
            if (shaderModules[i]->GetHandle() == VK_NULL_HANDLE)
            {
                delete pipeline;
                return VK_INCOMPLETE;
            }

            // Validate the shader module's entry point. Could either come from compilation of the GLSL source code or can be overridden by the VkPipelineShaderStageCreateInfo object.
            const std::string& shaderModuleEntryPoint = shaderModules[i]->GetEntryPoint();
            if (pCreateInfo->pStages[i].pEntryPoint) pipeline->m_entryPoints.push_back(std::string(pCreateInfo->pStages[i].pEntryPoint));
            else if (shaderModuleEntryPoint.size() > 0) pipeline->m_entryPoints.push_back(shaderModuleEntryPoint);
            // No entry point found so return error.
            else
            {
                delete pipeline;
                return VK_INCOMPLETE;
            }

            // Save the remaining shader module handle.
            pipeline->m_stages[i].module = pCreateInfo->pStages[i].module;

            // If specialization info exists for shader stage, copy the entries out.
            if (pCreateInfo->pStages[i].pSpecializationInfo)
            {
                SpecializationInfo specializationInfo = {};
                for (auto j = 0U; j < pCreateInfo->pStages[i].pSpecializationInfo->mapEntryCount; ++j)                
                    specializationInfo.mapEntries.push_back(pCreateInfo->pStages[i].pSpecializationInfo->pMapEntries[j]);

                specializationInfo.data.resize(pCreateInfo->pStages[i].pSpecializationInfo->dataSize);
                memcpy(specializationInfo.data.data(), pCreateInfo->pStages[i].pSpecializationInfo->pData, pCreateInfo->pStages[i].pSpecializationInfo->dataSize);
                
                pipeline->m_specializationInfo.emplace(shaderModules[i]->GetStage(), std::move(specializationInfo));
            }
        }

        // Merge shader stage resources into pipeline's.
        for (auto i = 0U; i < pipeline->m_stages.size(); ++i)
        {
            pipeline->MergeShaderResources(shaderModules[i]->GetResources());
        }

        // Separate all resources by set # which is required for descriptor set creation/management.
        pipeline->CreateSetBindings();

        // Create a descriptor set layout for pipeline creation and returning externally.
        auto result = pipeline->CreateDescriptorSetLayouts();
        if (result != VK_SUCCESS)
        {
            delete pipeline;
            return result;
        }

        // Create the pipeline layout handle.
        std::vector<VkDescriptorSetLayout> setLayouts;
        for (auto it : pipeline->m_descriptorSetLayouts)
            setLayouts.push_back(it.second->GetHandle());

        std::vector<VkPushConstantRange> pushConstantRanges;
        for (auto it : pipeline->m_resources)
        {
            const auto& resource = it.second;
            if (resource.resourceType == VEZ_PIPELINE_RESOURCE_TYPE_PUSH_CONSTANT_BUFFER)
            {
                VkPushConstantRange range = {};
                range.stageFlags = static_cast<VkShaderStageFlags>(resource.stages);
                range.offset = resource.offset;
                range.size = resource.size;
                pushConstantRanges.push_back(range);
            }
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        pipelineLayoutInfo.pSetLayouts = setLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
        pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
        result = vkCreatePipelineLayout(pDevice->GetHandle(), &pipelineLayoutInfo, nullptr, &pipeline->m_pipelineLayout);
        if (result != VK_SUCCESS)
        {
            delete pipeline;
            return result;
        }

        // Save pNext parameter.
       pipeline->m_pNext = pCreateInfo->pNext;

        // Return success.
        *ppPipeline = pipeline;
        return VK_SUCCESS;
    }

    VkResult Pipeline::Create(Device* pDevice, const VezComputePipelineCreateInfo* pCreateInfo, Pipeline** ppPipeline)
    {
        // Validate shader module was successfully created.
        auto shaderModule = ObjectLookup::GetImplShaderModule(pCreateInfo->pStage->module);
        if (shaderModule->GetHandle() == VK_NULL_HANDLE)
            return VK_INCOMPLETE;

        // Validate shader module is compute.
        if (shaderModule->GetStage() != VK_SHADER_STAGE_COMPUTE_BIT)
            return VK_INCOMPLETE;

        // Validate the shader module's entry point. Could either come from compilation of the GLSL source code or can be overridden by the VkPipelineShaderStageCreateInfo object.
        std::string shaderModuleEntryPoint = shaderModule->GetEntryPoint();
        if (pCreateInfo->pStage->pEntryPoint)
            shaderModuleEntryPoint = pCreateInfo->pStage->pEntryPoint;

        // If no entry point found then return an error.
        if (shaderModuleEntryPoint.size() == 0)
            return VK_INCOMPLETE;

        // Create a new instance of the Pipeline class.
        auto pipeline = new Pipeline;
        pipeline->m_device = pDevice;
        pipeline->m_bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

        // Copy the stage info.
        pipeline->m_stages.resize(1);
        pipeline->m_stages[0].module = pCreateInfo->pStage->module;
        pipeline->m_entryPoints.push_back(shaderModuleEntryPoint);

        // Merge shader stage resources into pipeline's.
        pipeline->MergeShaderResources(shaderModule->GetResources());

        // Separate all resources by set # which is required for descriptor set creation/management.
        pipeline->CreateSetBindings();

        // Create a descriptor set layout for pipeline creation and returning externally.
        auto result = pipeline->CreateDescriptorSetLayouts();
        if (result != VK_SUCCESS)
        {
            delete pipeline;
            return result;
        }

        // Create the pipeline layout handle.
        std::vector<VkDescriptorSetLayout> setLayouts;
        for (auto it : pipeline->m_descriptorSetLayouts)
            setLayouts.push_back(it.second->GetHandle());

        std::vector<VkPushConstantRange> pushConstantRanges;
        for (auto it : pipeline->m_resources)
        {
            const auto& resource = it.second;
            if (resource.resourceType == VEZ_PIPELINE_RESOURCE_TYPE_PUSH_CONSTANT_BUFFER)
            {
                VkPushConstantRange range = {};
                range.stageFlags = static_cast<VkShaderStageFlags>(resource.stages);
                range.offset = resource.offset;
                range.size = resource.size;
                pushConstantRanges.push_back(range);
            }
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        pipelineLayoutInfo.pSetLayouts = setLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
        pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
        result = vkCreatePipelineLayout(pDevice->GetHandle(), &pipelineLayoutInfo, nullptr, &pipeline->m_pipelineLayout);
        if (result != VK_SUCCESS)
        {
            delete pipeline;
            return result;
        }

        // Return success.
        *ppPipeline = pipeline;
        return VK_SUCCESS;
    }

    Pipeline::~Pipeline()
    {
        // Destroy pipeline layout.
        if (m_pipelineLayout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(m_device->GetHandle(), m_pipelineLayout, nullptr);

        // Destroy DescriptorSetLayout objects.
        for (auto it : m_descriptorSetLayouts)
            m_device->GetDescriptorSetLayoutCache()->DestroyLayout(it.second);
    }

    VkPipeline Pipeline::GetHandle(const RenderPass* pRenderPass, const GraphicsState* pState)
    {
        VkPipeline handle = VK_NULL_HANDLE;
        m_device->GetPipelineCache()->GetHandle(this, pRenderPass, pState, &handle);
        return handle;
    }

    VkResult Pipeline::EnumeratePipelineResources(uint32_t* pResourceCount, VezPipelineResource* pResources)
    {
        if (m_resources.size() == 0)
            return VK_INCOMPLETE;

        *pResourceCount = static_cast<uint32_t>(m_resources.size());
        if (pResources)
        {
            uint32_t i = 0;
            for (auto& it : m_resources)
            {
                memcpy(&pResources[i++], &it.second, sizeof(VezPipelineResource));
            }
        }

        return VK_SUCCESS;
    }

    VkResult Pipeline::GetPipelineResource(const char* name, VezPipelineResource* pResource)
    {
        // Iterate over set of resources to find specified name.
        for (auto it : m_resources)
        {
            // If resource is an output, remove the custom prefix from the key.
            auto key = it.first;
            if (key.find(':') != std::string::npos)
                key = key.substr(key.find(':') + 1);

            if (key == name)
            {
                memcpy(pResource, &it.second, sizeof(VezPipelineResource));
                return VK_SUCCESS;
            }
        }
        
        return VK_INCOMPLETE;
    }

    DescriptorSetLayout* Pipeline::GetDescriptorSetLayout(uint32_t setIndex)
    {
        auto it = m_descriptorSetLayouts.find(setIndex);
        if (it != m_descriptorSetLayouts.end())
        {
            return it->second;
        }
        else
        {
            return nullptr;
        }
    }

    VkAccessFlags Pipeline::GetBindingAccessFlags(uint32_t setIndex, uint32_t binding)
    {
        uint64_t key = (static_cast<uint64_t>(setIndex) << 32) | binding;
        auto it = m_bindingAccessFlags.find(key);
        if (it != m_bindingAccessFlags.end()) return it->second;
        else return VK_ACCESS_INDEX_READ_BIT;
    }

    VkShaderStageFlags Pipeline::GetPushConstantsRangeStages(uint32_t offset, uint32_t size)
    {
        // Iterate over all pipeline resources looking for push constant buffers.
        VkShaderStageFlags stages = 0;
        for (auto it : m_resources)
        {
            // If a push constant buffer is found, add it's shader stage flags to the return result if it falls within the specified byte range.
            if (it.second.resourceType == VEZ_PIPELINE_RESOURCE_TYPE_PUSH_CONSTANT_BUFFER)
            {
                if (offset >= it.second.offset && offset + size <= it.second.offset + it.second.size)
                    stages |= static_cast<VkShaderStageFlags>(it.second.stages);
            }
        }
        return stages;
    }

    uint32_t Pipeline::GetOutputsCount(VkShaderStageFlagBits shaderStage) const
    {
        uint32_t count = 0;

        for (auto it : m_resources)
        {
            if (it.second.stages == VK_SHADER_STAGE_FRAGMENT_BIT && it.second.resourceType == VEZ_PIPELINE_RESOURCE_TYPE_OUTPUT)
            {
                ++count;
            }
        }

        return count;
    }

    void Pipeline::MergeShaderResources(const std::vector<VezPipelineResource>& shaderResources)
    {
        // Iterate over all of the shader resources and merge into pipeline's resources map.
        for (auto& resource : shaderResources)
        {
            // The key used for each resource is its name, except in the case of outputs, since its legal to
            // have separate outputs with the same name across shader stages.
            auto key = std::string(resource.name);
            if (resource.resourceType == VEZ_PIPELINE_RESOURCE_TYPE_OUTPUT || resource.resourceType == VEZ_PIPELINE_RESOURCE_TYPE_INPUT)
                key = std::to_string(resource.stages) + ":" + key;

            // If resource already exists in pipeline resource map, add current stage's bit.
            // Else create a new entry in the pipeline resource map.
            auto it = m_resources.find(key);
            if (it != m_resources.end())
                it->second.stages |= resource.stages;
            else
                m_resources.emplace(key, resource);
        }
    }

    void Pipeline::CreateSetBindings()
    {
        for (auto& it : m_resources)
        {
            auto& resource = it.second;

            // Add to bindings map.
            auto it2 = m_bindings.find(resource.set);
            if (it2 != m_bindings.end())
            {
                it2->second.push_back(resource);
            }
            else
            {
                m_bindings.emplace(resource.set, std::vector<VezPipelineResource> { resource });
            }

            // Store access flags per binding (ignore INPUTS, OUTPUTS and PUSH_CONSTANTS).
            switch (resource.resourceType)
            {
            case VEZ_PIPELINE_RESOURCE_TYPE_INPUT:
            case VEZ_PIPELINE_RESOURCE_TYPE_OUTPUT:
            case VEZ_PIPELINE_RESOURCE_TYPE_PUSH_CONSTANT_BUFFER:
                continue;

            default:
            {
                uint64_t key = (static_cast<uint64_t>(resource.set) << 32) | resource.binding;
                m_bindingAccessFlags[key] = resource.access;
                break;
            }
            }
        }
    }

    VkResult Pipeline::CreateDescriptorSetLayouts()
    {
        // Iterate over each set binding and create a DescriptorSetLayout instance.
        for (auto it : m_bindings)
        {
            DescriptorSetLayout* descriptorSetLayout = nullptr;
            auto result = m_device->GetDescriptorSetLayoutCache()->CreateLayout(it.first, it.second, &descriptorSetLayout);
            if (result != VK_SUCCESS)
                return result;

            m_descriptorSetLayouts.emplace(it.first, descriptorSetLayout);
        }

        // Return success.
        return VK_SUCCESS;
    }    
}