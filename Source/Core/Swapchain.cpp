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
#include <limits>
#include <algorithm>
#include <array>
#include <cstring>
#include "Instance.h"
#include "PhysicalDevice.h"
#include "Device.h"
#include "Queue.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "SyncPrimitivesPool.h"
#include "Image.h"
#include "ImageView.h"
#include "Framebuffer.h"
#include "Swapchain.h"

namespace vez
{
    static SwapchainSupport QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        SwapchainSupport support;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &support.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount > 0)
        {
            support.formats.resize(formatCount);
            auto result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, support.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (presentModeCount > 0)
        {
            support.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, support.presentModes.data());
        }

        return support;
    }

    static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats, VkFormat format, VkColorSpaceKHR colorSpace)
    {
        if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
            return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

        for (const auto& availableFormat : availableFormats)
        {
            if (availableFormat.format == format && availableFormat.colorSpace == colorSpace)
                return availableFormat;
        }

        return availableFormats[0];
    }

    static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t clientWidth, uint32_t clientHeight)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            return capabilities.currentExtent;
        else
        {
            VkExtent2D actualExtent = { clientWidth, clientHeight };
            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
            return actualExtent;
        }
    }

    static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool vsyncEnabled)
    {
        // Try to match the correct present mode to the vsync state.
        std::vector<VkPresentModeKHR> desiredModes;
        if (vsyncEnabled) desiredModes = { VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_FIFO_RELAXED_KHR };
        else desiredModes = { VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR };
    
        // Iterate over all available present mdoes and match to one of the desired ones.
        for (const auto& availablePresentMode : availablePresentModes)
        {
            for (auto mode : desiredModes)
            {
                if (availablePresentMode == mode)
                    return availablePresentMode;
            }
        }

        // If no match was found, return the first present mode or default to FIFO.
        if (availablePresentModes.size() > 0) return availablePresentModes[0];
        else return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkResult Swapchain::Create(Device* pDevice, const VezSwapchainCreateInfo* pCreateInfo, Swapchain** ppSwapchain)
    {
        // Determine WSI support.
        VkBool32 supported = VK_FALSE;
        auto result = vkGetPhysicalDeviceSurfaceSupportKHR(pDevice->GetPhysicalDevice()->GetHandle(), 0, pCreateInfo->surface, &supported);
        if (result != VK_SUCCESS)
            return result;

        if (!supported)
            return VK_ERROR_INCOMPATIBLE_DISPLAY_KHR;

        // Query swapchain support on specified device and newly created surface.
        auto swapchainSupport = QuerySwapchainSupport(pDevice->GetPhysicalDevice()->GetHandle(), pCreateInfo->surface);
        if (swapchainSupport.formats.empty() || swapchainSupport.presentModes.empty())
            return VK_ERROR_INCOMPATIBLE_DISPLAY_KHR;

        // Initialize Swapchain class.
        auto swapchain = new Swapchain();
        swapchain->m_device = pDevice;
        memcpy(&swapchain->m_createInfo, pCreateInfo, sizeof(VezSwapchainCreateInfo));
        swapchain->m_surface = pCreateInfo->surface;
        swapchain->m_swapchainSupport = swapchainSupport;
        result = swapchain->Allocate();
        if (result != VK_SUCCESS)
        {
            delete swapchain;
            return result;
        }

        // Return success.
        *ppSwapchain = swapchain;
        return VK_SUCCESS;
    }

    Swapchain::~Swapchain()
    {
        if (m_handle)
            vkDestroySwapchainKHR(m_device->GetHandle(), m_handle, nullptr);
    }

    void Swapchain::FreeResources()
    {
        // Free semaphores used between swapchain blit and present.
        for (auto it : m_imageAcquiredSemaphores)
        {
            auto semaphore = it.second;
            m_device->GetSyncPrimitivesPool()->ReleaseSemaphores(1, &semaphore);
        }
    }

    VkResult Swapchain::AcquireNextImage(uint32_t* pImageIndex, VkSemaphore* pImageAcquiredSemaphore)
    {
        // If the surface extents changed since the last frame, the swapchain must be re-created.
        VkSurfaceCapabilitiesKHR surfaceCaps = {};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_device->GetPhysicalDevice()->GetHandle(), m_surface, &surfaceCaps);
        if (surfaceCaps.currentExtent.width != m_images[0]->GetCreateInfo().extent.width || surfaceCaps.currentExtent.height != m_images[0]->GetCreateInfo().extent.height)
        {
            // Wait for all device operations to finish and re-create swapchain.
            m_device->WaitIdle();
            Allocate();
        }

        // Acquire the next swapchain image.
        auto result = m_device->GetSyncPrimitivesPool()->AcquireSemaphore(1, pImageAcquiredSemaphore);
        if (result != VK_SUCCESS)
            return result;

        result = vkAcquireNextImageKHR(m_device->GetHandle(), m_handle, std::numeric_limits<uint64_t>::max(), *pImageAcquiredSemaphore, VK_NULL_HANDLE, pImageIndex);
        if (result != VK_SUCCESS)
        {
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
                result = Allocate();
        }

        if (result != VK_SUCCESS)
            return result;

        // Store the image acquired semaphore so it can be released next go around.
        m_imageAcquiredSemaphores[*pImageIndex] = *pImageAcquiredSemaphore;

        // Return success.
        return VK_SUCCESS;
    }

    VkResult Swapchain::SetVSync(VkBool32 enabled)
    {
        m_vsyncEnabled = enabled;
        return Allocate();
    }

    Image* Swapchain::GetImage(uint32_t index)
    {
        return (index < m_images.size()) ? m_images[index] : VK_NULL_HANDLE;
    }

    VkResult Swapchain::Allocate()
    {
        // Request current surface properties.
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_device->GetPhysicalDevice()->GetHandle(), m_surface, &m_swapchainSupport.capabilities);

        // Select the best color format and present mode based on what's available.
        auto surfaceFormat = ChooseSwapSurfaceFormat(m_swapchainSupport.formats, m_createInfo.format.format, m_createInfo.format.colorSpace);
        auto presentMode = ChooseSwapPresentMode(m_swapchainSupport.presentModes, m_vsyncEnabled);

        // Determine the total number of images required.
        uint32_t imageCount = (m_createInfo.tripleBuffer) ? 3 : m_swapchainSupport.capabilities.minImageCount + 1;
        if (m_swapchainSupport.capabilities.maxImageCount > 0 && imageCount > m_swapchainSupport.capabilities.maxImageCount)
            imageCount = m_swapchainSupport.capabilities.maxImageCount;

        // Get current dimensions of surface.
        auto extent = m_swapchainSupport.capabilities.currentExtent;

        // Create Vulkan handle.
        VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.surface = m_surface;
        swapchainCreateInfo.minImageCount = imageCount;
        swapchainCreateInfo.imageFormat = surfaceFormat.format;
        swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapchainCreateInfo.imageExtent = extent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        //! TODO This should be conditional on queue being used
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 1;
        uint32_t queueFamilyIndex = 0;
        swapchainCreateInfo.pQueueFamilyIndices = &queueFamilyIndex;

        swapchainCreateInfo.preTransform = m_swapchainSupport.capabilities.currentTransform;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = presentMode;
        swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.oldSwapchain = m_handle;

        VkSwapchainKHR handle = VK_NULL_HANDLE;
        auto result = vkCreateSwapchainKHR(m_device->GetHandle(), &swapchainCreateInfo, nullptr, &handle);
        if (result != VK_SUCCESS)
            return result;

        // Save final surface format.
        memcpy(&m_format, &surfaceFormat, sizeof(VkSurfaceFormatKHR));

        // Free previous handle.
        if (m_handle)
            vkDestroySwapchainKHR(m_device->GetHandle(), m_handle, nullptr);

        // Copy new swapchain handle.
        m_handle = handle;

        // Clear old list of images.
        m_images.clear();

        // Get the actual image count after swapchain is created since it could change.
        imageCount = 0U;
        vkGetSwapchainImagesKHR(m_device->GetHandle(), m_handle, &imageCount, nullptr);

        // Get the swapchain's image handles.
        std::vector<VkImage> images(imageCount);
        vkGetSwapchainImagesKHR(m_device->GetHandle(), m_handle, &imageCount, &images[0]);

        // Create an Image class instances to wrap swapchain image handles.
        for (auto handle : images)
        {
            VezImageCreateInfo imageCreateInfo = {};
            imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            imageCreateInfo.format = m_createInfo.format.format;
            imageCreateInfo.extent = { extent.width, extent.height, 1 };
            imageCreateInfo.mipLevels = 1;
            imageCreateInfo.arrayLayers = 1;
            imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            auto image = Image::CreateFromHandle(m_device, &imageCreateInfo, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, handle, nullptr);
            m_images.push_back(image);

            // Transition image to default layout.
            m_device->TransitionImageLayout(m_images.back(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        }

        // Return success.
        return VK_SUCCESS;
    }
}