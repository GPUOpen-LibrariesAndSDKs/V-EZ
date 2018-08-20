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
#include <thread>
#include "VEZ.h"
#include "Utility/VkHelpers.h"

namespace vez
{
    class Device;
    class DescriptorPool;

    class DescriptorSetLayout
    {
    public:
        static VkResult Create(Device* device, const DescriptorSetLayoutHash& hash, const std::vector<VezPipelineResource>& setResources, DescriptorSetLayout** pLayout);

        ~DescriptorSetLayout();

        Device* GetDevice() { return m_device; }

        VkDescriptorSetLayout GetHandle() const { return m_handle; }

        std::vector<VkDescriptorSetLayoutBinding> GetBindings() { return m_bindings; }

        const DescriptorSetLayoutHash& GetHash() const { return m_hash; }

        bool GetLayoutBinding(uint32_t bindingIndex, VkDescriptorSetLayoutBinding** pBinding);

        VkDescriptorSet AllocateDescriptorSet();

        VkResult FreeDescriptorSet(VkDescriptorSet descriptorSet);

    private:
        Device* m_device = nullptr;
        DescriptorSetLayoutHash m_hash;
        VkDescriptorSetLayout m_handle = VK_NULL_HANDLE;
        std::vector<VkDescriptorSetLayoutBinding> m_bindings;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_bindingsLookup;
        DescriptorPool* m_descriptorPool;
    };    
}