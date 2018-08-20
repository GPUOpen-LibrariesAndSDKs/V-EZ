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
#include <queue>
#include <unordered_map>
#include "VEZ.h"

namespace vez
{
    class Device;
    class CommandBuffer;
    class Image;

    struct SwapchainSupport
    {
        VkSurfaceCapabilitiesKHR capabilities = { };
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    class Swapchain
    {
    public:
        static VkResult Create(Device* pDevice, const VezSwapchainCreateInfo* pCreateInfo, Swapchain** ppSwapchain);

        ~Swapchain();

        void FreeResources();

        VkSwapchainKHR GetHandle() { return m_handle; }

        const VezSwapchainCreateInfo& GetCreateInfo() { return m_createInfo; }

        VkSurfaceFormatKHR GetFormat() { return m_format; }

        VkSurfaceKHR GetSurface() const { return m_surface; }

        VkResult AcquireNextImage(uint32_t* pImageIndex, VkSemaphore* pImageAcquiredSemaphore);

        VkResult SetVSync(VkBool32 enabled);

        Image* GetImage(uint32_t index);

    private:
        VkResult Allocate();

        Device* m_device = nullptr;
        VezSwapchainCreateInfo m_createInfo = { };
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        SwapchainSupport m_swapchainSupport;
        VkSwapchainKHR m_handle = VK_NULL_HANDLE;
        VkSurfaceFormatKHR m_format = { };
        VkBool32 m_vsyncEnabled = VK_FALSE;
        std::vector<Image*> m_images;
        std::unordered_map<uint32_t, VkSemaphore> m_imageAcquiredSemaphores;
    };    
}