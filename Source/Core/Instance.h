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
#include <string>
#include <vector>
#include "VEZ.h"

namespace vez
{
    class ThreadPool;
    class PhysicalDevice;

    class Instance
    {
    public:
        static VkResult Create(const VezInstanceCreateInfo* pCreateInfo, Instance** ppInstance);

        static void Destroy(Instance* pInstance);

        VkInstance GetHandle() const { return m_handle; }

        const std::vector<PhysicalDevice*>& GetPhysicalDevices() { return m_physicalDevices; }

        ThreadPool* GetThreadPool() { return m_threadPool; }

    private:
        VkInstance m_handle = VK_NULL_HANDLE;
        std::vector<std::string> m_validationLayers;
        std::vector<PhysicalDevice*> m_physicalDevices;
        ThreadPool* m_threadPool = nullptr;
    };    
}