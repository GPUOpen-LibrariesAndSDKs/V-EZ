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
#include <map>
#include <unordered_map>
#include "Utility/SpinLock.h"
#include "VEZ.h"

namespace vez
{
    class DescriptorSetLayout;

    class DescriptorPool
    {
    public:
        DescriptorPool(DescriptorSetLayout* layout);

        ~DescriptorPool();

        VkDescriptorSet AllocateDescriptorSet();

        VkResult FreeDescriptorSet(VkDescriptorSet descriptorSet);

    private:
        DescriptorSetLayout* m_layout = nullptr;
        std::vector<VkDescriptorPoolSize> m_poolSizes;
        std::vector<VkDescriptorPool> m_pools;
        std::vector<uint32_t> m_allocatedSets;
        uint32_t m_currentAllocationPoolIndex = 0;
        uint32_t m_maxSetsPerPool = 50;

        std::unordered_map<VkDescriptorSet, uint32_t> m_allocatedDescriptorSets;
        SpinLock m_spinLock;
    };
}