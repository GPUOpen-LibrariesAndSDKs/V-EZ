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
#include <fstream>
#include <array>
#include <vector>
#include <unordered_map>
#include "Window.h"

static std::unordered_map<GLFWwindow*, Window*> windowInstances;

void Window::WindowSizeCallback(GLFWwindow* window, int width, int height)
{
    auto itr = windowInstances.find(window);
    if (itr != windowInstances.end())
    {
        // Wait for device to be idle.
        auto window = itr->second;
        vkDeviceWaitIdle(window->m_device);

        // Re-create the framebuffer.
        window->CreateFramebuffer();
    }
}

Window::Window(VkInstance instance, VkDevice device, const char* title, int width, int height)
    : m_instance(instance)
    , m_device(device)
{
    // Initialize a window using GLFW and hint no graphics API should be used on the backend.
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    windowInstances[m_window] = this;

    // Create a surface from the GLFW window handle.
    auto result = glfwCreateWindowSurface(instance, m_window, nullptr, &m_surface);
    if (result != VK_SUCCESS)
        FATAL("glfwCreateWindowSurface failed!");

    // Create the swapchain.
    VezSwapchainCreateInfo swapchainCreateInfo = {};
    swapchainCreateInfo.surface = m_surface;
    swapchainCreateInfo.format = { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    result = vezCreateSwapchain(m_device, &swapchainCreateInfo, &m_swapchain);
    if (result != VK_SUCCESS)
        FATAL("vezCreateSwapchain failed");

    // Create a framebuffer to render into.
    CreateFramebuffer();
}

Window::~Window()
{
    // Destroy framebuffer.
    vezDestroyImageView(m_device, m_framebuffer.depthStencilImageView);
    vezDestroyImageView(m_device, m_framebuffer.colorImageView);
    vezDestroyImage(m_device, m_framebuffer.depthStencilImage);
    vezDestroyImage(m_device, m_framebuffer.colorImage);
    vezDestroyFramebuffer(m_device, m_framebuffer.handle);

    // Destroy the swapchain.
    vezDestroySwapchain(m_device, m_swapchain);

    // Destroy surface.
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
}

void Window::GetWindowSize(uint32_t* width, uint32_t* height)
{
    if (m_window)
    {
        int w, h;
        glfwGetWindowSize(m_window, &w, &h);
        *width = static_cast<uint32_t>(w);
        *height = static_cast<uint32_t>(h);
    }
}

bool Window::IsClosed()
{
    return (glfwWindowShouldClose(m_window) != 0);
}

void Window::CreateFramebuffer()
{
    // Free previous allocations.
    if (m_framebuffer.handle)
    {
        vezDestroyFramebuffer(m_device, m_framebuffer.handle);
        vezDestroyImageView(m_device, m_framebuffer.colorImageView);
        vezDestroyImageView(m_device, m_framebuffer.depthStencilImageView);
        vezDestroyImage(m_device, m_framebuffer.colorImage);
        vezDestroyImage(m_device, m_framebuffer.depthStencilImage);
    }

    // Get the current window dimension.
    int width, height;
    glfwGetWindowSize(m_window, &width, &height);

    // Match the swapchain's image format.
    VkSurfaceFormatKHR swapchainFormat = {};
    vezGetSwapchainSurfaceFormat(m_swapchain, &swapchainFormat);

    // Create the color image for the m_framebuffer.
    VezImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = swapchainFormat.format;
    imageCreateInfo.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    auto result = vezCreateImage(m_device, VEZ_MEMORY_GPU_ONLY, &imageCreateInfo, &m_framebuffer.colorImage);
    if (result != VK_SUCCESS)
        FATAL("vezCreateImage failed");

    // Create the image view for binding the texture as a resource.
    VezImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.image = m_framebuffer.colorImage;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = imageCreateInfo.format;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    result = vezCreateImageView(m_device, &imageViewCreateInfo, &m_framebuffer.colorImageView);
    if (result != VK_SUCCESS)
        FATAL("vezCreateImageView failed");

    // Create the depth image for the m_framebuffer.
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_D32_SFLOAT;
    imageCreateInfo.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    result = vezCreateImage(m_device, VEZ_MEMORY_GPU_ONLY, &imageCreateInfo, &m_framebuffer.depthStencilImage);
    if (result != VK_SUCCESS)
        FATAL("vezCreateImage failed");

    // Create the image view for binding the texture as a resource.
    imageViewCreateInfo.image = m_framebuffer.depthStencilImage;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = imageCreateInfo.format;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    result = vezCreateImageView(m_device, &imageViewCreateInfo, &m_framebuffer.depthStencilImageView);
    if (result != VK_SUCCESS)
        FATAL("vezCreateImageView failed");

    // Create the m_framebuffer.
    std::array<VkImageView, 2> attachments = { m_framebuffer.colorImageView, m_framebuffer.depthStencilImageView };
    VezFramebufferCreateInfo framebufferCreateInfo = {};
    framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferCreateInfo.pAttachments = attachments.data();
    framebufferCreateInfo.width = width;
    framebufferCreateInfo.height = height;
    framebufferCreateInfo.layers = 1;
    result = vezCreateFramebuffer(m_device, &framebufferCreateInfo, &m_framebuffer.handle);
    if (result != VK_SUCCESS)
        FATAL("vezCreateFramebuffer failed");
}