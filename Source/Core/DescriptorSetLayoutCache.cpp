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
#include "Utility/SpinLock.h"
#include "Utility/VkHelpers.h"
#include "Device.h"
#include "DescriptorSetLayout.h"
#include "DescriptorSetLayoutCache.h"

namespace vez
{
    static DescriptorSetLayoutHash GetHash(uint32_t setIndex, const std::vector<VezPipelineResource>& setResources)
    {
        // Descriptor set layout binding bit field declaration for generating hash.
        struct DescriptorSetLayoutBindingBitField
        {
            uint32_t descriptorCount : 32;
            uint16_t binding : 16;
            uint8_t descriptorType : 4;
            uint8_t stageFlags : 6;
        };

        // Generate bit field entries for each descriptor set resource.
        DescriptorSetLayoutHash hash(1 + setResources.size() * (sizeof(DescriptorSetLayoutBindingBitField) / 4));
        hash[0] = setIndex;
        DescriptorSetLayoutBindingBitField* bitfield = reinterpret_cast<DescriptorSetLayoutBindingBitField*>(&hash[1]);
        for (auto i = 0U; i < setResources.size(); ++i)
        {
            bitfield->binding = setResources[i].binding;
            bitfield->descriptorType = setResources[i].resourceType;
            bitfield->descriptorCount = setResources[i].arraySize;
            bitfield->stageFlags = setResources[i].stages;
            
            ++bitfield;
        }

        return hash;
    }

    DescriptorSetLayoutCache::DescriptorSetLayoutCache(Device* device)
        : m_device(device)
    {

    }

    DescriptorSetLayoutCache::~DescriptorSetLayoutCache()
    {
        for (auto it : m_layouts)
        {
            delete it.second;
        }
    }

    VkResult DescriptorSetLayoutCache::CreateLayout(uint32_t setIndex, const std::vector<VezPipelineResource>& setResources, DescriptorSetLayout** pLayout)
    {
        // Generate hash from resource layout.
        auto hash = GetHash(setIndex, setResources);

        // Find or create a DescriptorSetLayout instance for the given descriptor set resouces.  Make thread-safe.
        DescriptorSetLayout* descriptorSetLayout = nullptr;
        VkResult result = VK_SUCCESS;

        // Acquire access to the cache.
        m_spinLock.Lock();

        // Look up hash.
        auto it = m_layouts.find(hash);
        if (it != m_layouts.end())
        {
            descriptorSetLayout = it->second;
            ++m_layoutReferences[descriptorSetLayout];
        }
        else
        {
            auto result = DescriptorSetLayout::Create(m_device, hash, setResources, &descriptorSetLayout);
            if (result == VK_SUCCESS)
            {
                m_layouts.emplace(std::move(hash), descriptorSetLayout);
                m_layoutReferences.emplace(descriptorSetLayout, 1U);
            }
        }

        // Release access to the cache.
        m_spinLock.Unlock();

        // Return result.
        if (result == VK_SUCCESS)
            *pLayout = descriptorSetLayout;

        return result;
    }

    void DescriptorSetLayoutCache::DestroyLayout(DescriptorSetLayout* layout)
    {
#if 0
        // Acquire access to the cache.
        m_spinLock.Lock();

        // Look up the layout's hash.
        const auto& hash = layout->GetHash();
        auto it = m_layouts.find(hash);
        if (it != m_layouts.end())
        {
            // Decrement reference count.
            auto& refCount = m_layoutReferences.at(layout);
            if (--refCount == 0)
            {
                m_layouts.erase(it);
                m_layoutReferences.erase(layout);
                delete layout;
            }
        }

        // Release access to the cache.
        m_spinLock.Unlock();
#endif
    }
}
