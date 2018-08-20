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
#include "Utility/VkHelpers.h"
#include "Utility/SpinLock.h"
#include "Device.h"
#include "DescriptorSetLayout.h"
#include "DescriptorPool.h"

namespace vez
{
    DescriptorPool::DescriptorPool(DescriptorSetLayout* layout)
        : m_layout(layout)
    {
        // Get the layout's binding information.
        const auto& bindings = layout->GetBindings();

        // Generate array of pool sizes for each unique resource type.
        std::unordered_map<VkDescriptorType, uint32_t> descriptorTypeCounts;
        for (auto& binding : bindings)
        {
            descriptorTypeCounts[binding.descriptorType] += binding.descriptorCount;
        }

        // Fill in pool sizes array.
        m_poolSizes.resize(descriptorTypeCounts.size());
        uint32_t index = 0;
        for (auto& it : descriptorTypeCounts)
        {
            m_poolSizes[index].type = it.first;
            m_poolSizes[index].descriptorCount = it.second * m_maxSetsPerPool;
            ++index;
        }
    }

    DescriptorPool::~DescriptorPool()
    {
        // Destroy all allocated descriptor sets.
        for (auto it : m_allocatedDescriptorSets)
        {
            vkFreeDescriptorSets(m_layout->GetDevice()->GetHandle(), m_pools[it.second], 1, &it.first);
        }

        // Destroy all created pools.
        for (auto pool : m_pools)
        {
            vkDestroyDescriptorPool(m_layout->GetDevice()->GetHandle(), pool, nullptr);
        }
    }

    VkDescriptorSet DescriptorPool::AllocateDescriptorSet()
    {
        // Safe guard access to internal resources across threads.
        m_spinLock.Lock();

        // Find the next pool to allocate from.
        while (true)
        {
            // Allocate a new VkDescriptorPool if necessary.
            if (m_pools.size() <= m_currentAllocationPoolIndex)
            {
                // Create the Vulkan descriptor pool.
                VkDescriptorPoolCreateInfo createInfo = {};
                createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
                createInfo.poolSizeCount = static_cast<uint32_t>(m_poolSizes.size());
                createInfo.pPoolSizes = m_poolSizes.data();
                createInfo.maxSets = m_maxSetsPerPool;
                VkDescriptorPool handle = VK_NULL_HANDLE;
                auto result = vkCreateDescriptorPool(m_layout->GetDevice()->GetHandle(), &createInfo, nullptr, &handle);
                if (result != VK_SUCCESS)
                    return VK_NULL_HANDLE;

                // Add the Vulkan handle to the descriptor pool instance.
                m_pools.push_back(handle);
                m_allocatedSets.push_back(0);
                break;
            }
            else if (m_allocatedSets[m_currentAllocationPoolIndex] < m_maxSetsPerPool)
                break;

            // Increment pool index.
            ++m_currentAllocationPoolIndex;
        }

        // Increment allocated set count for given pool.
        ++m_allocatedSets[m_currentAllocationPoolIndex];

        // Allocate a new descriptor set from the current pool index.
        VkDescriptorSetLayout setLayout = m_layout->GetHandle();

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_pools[m_currentAllocationPoolIndex];
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &setLayout;
        VkDescriptorSet handle = VK_NULL_HANDLE;
        auto result = vkAllocateDescriptorSets(m_layout->GetDevice()->GetHandle(), &allocInfo, &handle);
        if (result != VK_SUCCESS)
            return VK_NULL_HANDLE;

        // Store an internal mapping between the descriptor set handle and it's parent pool.
        // This is used when FreeDescriptorSet is called downstream.
        m_allocatedDescriptorSets.emplace(handle, m_currentAllocationPoolIndex);

        // Unlock access to internal resources.
        m_spinLock.Unlock();

        // Return descriptor set handle.
        return handle;
    }

    VkResult DescriptorPool::FreeDescriptorSet(VkDescriptorSet descriptorSet)
    {
        // Safe guard access to internal resources across threads.
        m_spinLock.Lock();

        // Get the index of the descriptor pool the descriptor set was allocated from.
        auto it = m_allocatedDescriptorSets.find(descriptorSet);
        if (it == m_allocatedDescriptorSets.end())
            return VK_INCOMPLETE;

        // Return the descriptor set to the original pool.
        auto poolIndex = it->second;
        vkFreeDescriptorSets(m_layout->GetDevice()->GetHandle(), m_pools[poolIndex], 1, &descriptorSet);

        // Remove descriptor set from allocatedDescriptorSets map.
        m_allocatedDescriptorSets.erase(descriptorSet);

        // Decrement the number of allocated descriptor sets for the pool.
        --m_allocatedSets[poolIndex];

        // Set the next allocation to use this pool index.
        m_currentAllocationPoolIndex = poolIndex;

        // Unlock access to internal resources.
        m_spinLock.Unlock();

        // Return success.
        return VK_SUCCESS;
    }    
}