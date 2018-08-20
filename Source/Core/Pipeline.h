//
// Copyright (c) 2017 Advanced Micro Devices, Inc. All rights reserved.
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
//
// @author Sean O'Connell (Sean.OConnell@amd.com)
//
#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include "VEZ.h"
#include "GraphicsState.h"

namespace vez
{
    class Device;
    class DescriptorSetLayout;
    class RenderPass;

    struct SpecializationInfo
    {
        std::vector<VkSpecializationMapEntry> mapEntries;
        std::vector<uint8_t> data;
    };

    class Pipeline
    {
    public:
        static VkResult Create(Device* pDevice, const VezGraphicsPipelineCreateInfo* pCreateInfo, Pipeline** ppPipeline);

        static VkResult Create(Device* pDevice, const VezComputePipelineCreateInfo* pCreateInfo, Pipeline** ppPipeline);

        ~Pipeline();

        const std::string& GetInfoLog() const { return m_infoLog; }

        const std::unordered_map<uint32_t, std::vector<VezPipelineResource>>& GetBindings() const { return m_bindings; }

        VkPipelineBindPoint GetBindPoint() const { return m_bindPoint; }

        VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }

        VkPipeline GetHandle(const RenderPass* pRenderPass, const GraphicsState* pState);

        VkResult EnumeratePipelineResources(uint32_t* pResourceCount, VezPipelineResource* pResources);

        VkResult GetPipelineResource(const char* name, VezPipelineResource* pResource);

        DescriptorSetLayout* GetDescriptorSetLayout(uint32_t setIndex);

        VkAccessFlags GetBindingAccessFlags(uint32_t setIndex, uint32_t binding);

        VkShaderStageFlags GetPushConstantsRangeStages(uint32_t offset, uint32_t size);

        uint32_t GetOutputsCount(VkShaderStageFlagBits shaderStage) const;

    private:
        void MergeShaderResources(const std::vector<VezPipelineResource>& shaderResources);

        void CreateSetBindings();

        VkResult CreateDescriptorSetLayouts();

        Device* m_device = nullptr;
        VkPipelineBindPoint m_bindPoint;
        const void* m_pNext;
        std::vector<VezPipelineShaderStageCreateInfo> m_stages;
        std::vector<std::string> m_entryPoints;
        std::unordered_map<VkShaderStageFlagBits, SpecializationInfo> m_specializationInfo;

        std::map<std::string, VezPipelineResource> m_resources;
        std::unordered_map<uint32_t, std::vector<VezPipelineResource>> m_bindings;
        std::unordered_map<uint32_t, DescriptorSetLayout*> m_descriptorSetLayouts;
        std::unordered_map<uint64_t, VkAccessFlags> m_bindingAccessFlags;
        
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
        
        std::string m_infoLog;

        friend class PipelineCache;
    };    
}