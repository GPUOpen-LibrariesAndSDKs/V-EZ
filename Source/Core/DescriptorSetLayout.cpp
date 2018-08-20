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
#include <thread>
#include "Utility/VkHelpers.h"
#include "Device.h"
#include "DescriptorPool.h"
#include "DescriptorSetLayout.h"

namespace vez
{
    VkResult DescriptorSetLayout::Create(Device* device, const DescriptorSetLayoutHash& hash, const std::vector<VezPipelineResource>& setResources, DescriptorSetLayout** pLayout)
    {
        // Create a new DescriptorPool instance.
        auto descriptorSetLayout = new DescriptorSetLayout;
        descriptorSetLayout->m_device = device;
        descriptorSetLayout->m_hash = hash;

        // Static mapping between VulkanEZ pipeline resource types to Vulkan descriptor types.
        std::unordered_map<VezPipelineResourceType, VkDescriptorType> resourceTypeMapping = {
            { VEZ_PIPELINE_RESOURCE_TYPE_SAMPLER, VK_DESCRIPTOR_TYPE_SAMPLER },
            { VEZ_PIPELINE_RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER },
            { VEZ_PIPELINE_RESOURCE_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE },
            { VEZ_PIPELINE_RESOURCE_TYPE_STORAGE_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE },
            { VEZ_PIPELINE_RESOURCE_TYPE_UNIFORM_TEXEL_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER },
            { VEZ_PIPELINE_RESOURCE_TYPE_STORAGE_TEXEL_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER },
            { VEZ_PIPELINE_RESOURCE_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
            { VEZ_PIPELINE_RESOURCE_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
            { VEZ_PIPELINE_RESOURCE_TYPE_INPUT_ATTACHMENT, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT },
        };

        // Extract all unique resource types and their counts as well as layout bindings.
        for (auto& resource : setResources)
        {
            // Skip shader inputs and outputs (NOTE: might be required later at some point).
            // Also other types like subpass inputs, pushconstants, etc. should eventually be handled.
            switch (resource.resourceType)
            {
            case VEZ_PIPELINE_RESOURCE_TYPE_INPUT:
            case VEZ_PIPELINE_RESOURCE_TYPE_OUTPUT:
            case VEZ_PIPELINE_RESOURCE_TYPE_PUSH_CONSTANT_BUFFER:
                continue;

            default:
                break;
            }

            // Convert from VkPipelineResourceType to VkDescriptorType.
            auto descriptorType = resourceTypeMapping.at(resource.resourceType);

            // Populate the Vulkan binding info struct.
            VkDescriptorSetLayoutBinding bindingInfo = {};
            bindingInfo.binding = resource.binding;
            bindingInfo.descriptorCount = resource.arraySize;
            bindingInfo.descriptorType = resourceTypeMapping.at(resource.resourceType);
            bindingInfo.stageFlags = static_cast<VkShaderStageFlags>(resource.stages);
            descriptorSetLayout->m_bindings.push_back(bindingInfo);

            // Create a quick lookup table for each binding point.
            descriptorSetLayout->m_bindingsLookup.emplace(resource.binding, bindingInfo);
        }

        // Create the Vulkan descriptor set layout handle.
        VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutCreateInfo.bindingCount = static_cast<uint32_t>(descriptorSetLayout->m_bindings.size());
        layoutCreateInfo.pBindings = descriptorSetLayout->m_bindings.data();
        auto result = vkCreateDescriptorSetLayout(descriptorSetLayout->m_device->GetHandle(), &layoutCreateInfo, nullptr, &descriptorSetLayout->m_handle);
        if (result != VK_SUCCESS)
        {
            delete descriptorSetLayout;
            return result;
        }

        // Allocate a DescriptorPool from the new instance.
        descriptorSetLayout->m_descriptorPool = new DescriptorPool(descriptorSetLayout);

        // Save handle.
        *pLayout = descriptorSetLayout;

        // Return success.
        return VK_SUCCESS;
    }

    DescriptorSetLayout::~DescriptorSetLayout()
    {
        vkDestroyDescriptorSetLayout(m_device->GetHandle(), m_handle, nullptr);
        delete m_descriptorPool;
    }

    bool DescriptorSetLayout::GetLayoutBinding(uint32_t bindingIndex, VkDescriptorSetLayoutBinding** pBinding)
    {
        auto it = m_bindingsLookup.find(bindingIndex);
        if (it == m_bindingsLookup.end())
            return false;

        *pBinding = &it->second;
        return true;
    }

    VkDescriptorSet DescriptorSetLayout::AllocateDescriptorSet()
    {
        // Return new descriptor set allocation.
        return m_descriptorPool->AllocateDescriptorSet();
    }

    VkResult DescriptorSetLayout::FreeDescriptorSet(VkDescriptorSet descriptorSet)
    {
        // Free descriptor set handle.
        return m_descriptorPool->FreeDescriptorSet(descriptorSet);
    }
}