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
#include "ResourceBindings.h"

namespace vez
{
    void ResourceBindings::Clear(uint32_t set)
    {
        m_setBindings.erase(set);
    }

    void ResourceBindings::Reset()
    {
        m_setBindings.clear();
        m_dirty = false;
    }

    void ResourceBindings::BindBuffer(Buffer* pBuffer, VkDeviceSize offset, VkDeviceSize range, uint32_t set, uint32_t binding, uint32_t arrayElement)
    {
        Bind(set, binding, arrayElement, BindingInfo{ offset, range, pBuffer, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, true });
    }

    void ResourceBindings::BindBufferView(BufferView* pBufferView, uint32_t set, uint32_t binding, uint32_t arrayElement)
    {
        Bind(set, binding, arrayElement, BindingInfo{ 0, 0, VK_NULL_HANDLE, pBufferView, VK_NULL_HANDLE, VK_NULL_HANDLE, true });
    }

    void ResourceBindings::BindImageView(ImageView* pImageView, VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement)
    {
        Bind(set, binding, arrayElement, BindingInfo{ 0, 0, VK_NULL_HANDLE, VK_NULL_HANDLE, pImageView, sampler, true });
    }

    void ResourceBindings::BindSampler(VkSampler sampler, uint32_t set, uint32_t binding, uint32_t arrayElement)
    {
        Bind(set, binding, arrayElement, BindingInfo{ 0, 0, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, sampler, true });
    }

    void ResourceBindings::Bind(uint32_t set, uint32_t binding, uint32_t arrayElement, const BindingInfo& info)
    {
        // If resource is being removed from binding, erase the entry.
        if (info.pBuffer == VK_NULL_HANDLE && info.pBufferView == VK_NULL_HANDLE && info.pImageView == VK_NULL_HANDLE && info.sampler == VK_NULL_HANDLE)
        {
            auto it = m_setBindings.find(set);
            if (it != m_setBindings.end())
            {
                auto& setBindings = it->second;
                auto it2 = setBindings.bindings.find(binding);
                if (it2 != setBindings.bindings.end())
                {
                    auto& arrayBindings = it2->second;
                    auto it3 = arrayBindings.find(arrayElement);
                    if (it3 != arrayBindings.end())
                    {
                        arrayBindings.erase(it3);

                        if (arrayBindings.size() == 0)
                            setBindings.bindings.erase(it2);

                        setBindings.dirty = true;
                    }
                }
            }
        }
        // Else binding is being added.
        else
        {
            // If set # does not exist yet, add it.
            auto it = m_setBindings.find(set);
            if (it == m_setBindings.end())
            {
                ArrayBindings arrayBinding = { { arrayElement, info } };

                SetBindings setBindings;
                setBindings.bindings.emplace(binding, std::move(arrayBinding));
                setBindings.dirty = true;

                m_setBindings.emplace(set, std::move(setBindings));
            }
            // Else, find the binding #.
            else
            {
                // If the binding # does not exist, create it and add the array element.
                auto& setBinding = it->second;
                auto it2 = setBinding.bindings.find(binding);
                if (it2 == setBinding.bindings.end())
                {
                    ArrayBindings arrayBinding = { { arrayElement, info } };
                    setBinding.bindings.emplace(binding, std::move(arrayBinding));
                }
                else
                {
                    it2->second[arrayElement] = info;
                    setBinding.dirty = true;
                }
            }
        }

        // Always mark ResourceBindings as dirty for fast checking during descriptor set binding.
        m_dirty = true;
    }    
}