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
#include <tuple>
#include <vector>
#include <iostream>
#include <VEZ.h>
#include <GLFW/glfw3.h>

#define FATAL(msg) { std::cout << __FUNCTION__ << " (" << __LINE__ << ") " << msg << "\n"; exit(1); }

class Window
{
public:
    Window(VkInstance instance, VkDevice device, const char* title, int width, int height);
    virtual ~Window();
    
    VezSwapchain GetSwapchain() const { return m_swapchain; }
    VkImage GetColorAttachment() { return m_framebuffer.colorImage; }
    VkImageView GetColorAttachmentView() { return m_framebuffer.colorImageView; }
    VezFramebuffer GetFramebuffer() { return m_framebuffer.handle; }
    void GetWindowSize(uint32_t* width, uint32_t* height);
    bool IsClosed();

protected:
    void CreateFramebuffer();

private:
    static void WindowSizeCallback(GLFWwindow* window, int width, int height);

    VkInstance m_instance = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    GLFWwindow* m_window = nullptr;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VezSwapchain m_swapchain = VK_NULL_HANDLE;

    struct {
        VkImage colorImage = VK_NULL_HANDLE;
        VkImageView colorImageView = VK_NULL_HANDLE;
        VkImage depthStencilImage = VK_NULL_HANDLE;
        VkImageView depthStencilImageView = VK_NULL_HANDLE;
        VezFramebuffer handle = VK_NULL_HANDLE;
    } m_framebuffer;
};