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
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include "Utility/ThreadPool.h"
#include "PhysicalDevice.h"
#include "Instance.h"

namespace vez
{
    VkResult Instance::Create(const VezInstanceCreateInfo* pCreateInfo, Instance** ppInstance)
    {
        // Fill out VkApplicationInfo struct.
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext = pCreateInfo->pApplicationInfo->pNext;
        appInfo.pApplicationName = pCreateInfo->pApplicationInfo->pApplicationName;
        appInfo.applicationVersion = pCreateInfo->pApplicationInfo->applicationVersion;
        appInfo.pEngineName = pCreateInfo->pApplicationInfo->pEngineName;
        appInfo.engineVersion = pCreateInfo->pApplicationInfo->engineVersion;
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // Create VkInstance.
        VkInstanceCreateInfo instanceCreateInfo = {};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pNext = pCreateInfo->pNext;
        instanceCreateInfo.pApplicationInfo = &appInfo;
        instanceCreateInfo.enabledLayerCount = pCreateInfo->enabledLayerCount;
        instanceCreateInfo.ppEnabledLayerNames = pCreateInfo->ppEnabledLayerNames;
        instanceCreateInfo.enabledExtensionCount = pCreateInfo->enabledExtensionCount;
        instanceCreateInfo.ppEnabledExtensionNames = pCreateInfo->ppEnabledExtensionNames;

        VkInstance handle = VK_NULL_HANDLE;
        auto result = vkCreateInstance(&instanceCreateInfo, nullptr, &handle);
        if (result != VK_SUCCESS)
            return result;

        // Create a new Instance object to wrap Vulkan handle.
        auto instance = new Instance();
        instance->m_handle = handle;

        // Get the number of attached physical devices.
        uint32_t physicalDeviceCount = 0;
        result = vkEnumeratePhysicalDevices(handle, &physicalDeviceCount, nullptr);
        if (result != VK_SUCCESS)
            return result;

        // Make sure there is at least one physical device present.
        if (physicalDeviceCount > 0)
        {
            // Enumerate physical device handles.
            std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
            result = vkEnumeratePhysicalDevices(handle, &physicalDeviceCount, physicalDevices.data());
            if (result != VK_SUCCESS)
                return result;

            // Wrap native Vulkan handles in PhysicalDevice class.
            for (auto& pd : physicalDevices)
            {
                instance->m_physicalDevices.emplace_back(instance, pd);
            }
        }

        // Initialize the Instance's thread pool with a single worker thread for now.
        // TODO: Assess background tasks and performance before increasing thread count.
        instance->m_threadPool = new ThreadPool(1);

        // Copy address of object instance.
        *ppInstance = instance;

        // Return success.
        return VK_SUCCESS;
    }

    void Instance::Destroy(Instance* pInstance)
    {
        delete pInstance->m_threadPool;
        vkDestroyInstance(pInstance->GetHandle(), nullptr);
        delete pInstance;
    }
}