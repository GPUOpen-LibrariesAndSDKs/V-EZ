//
// Copyright (c) 2017 Advanced Micro Devices, Inc. All rights reserved.
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
//
// @author Sean O'Connell (Sean.OConnell@amd.com)
//
#pragma once

#include <string>
#include <tuple>
#ifndef VKEZ_NO_PROTOTYPES
#include <VEZ.h>
#else
#include "VulkanEZ_Loader.h"
#endif
#include <GLFW/glfw3.h>

class AppBase
{
public:
    AppBase(const char* name, int width, int height, int physicalDeviceIndex = 0, bool manageFramebuffer = true, VkSampleCountFlagBits sampleCountFlag = VK_SAMPLE_COUNT_1_BIT);
    virtual ~AppBase();

    int Run();
    void Quit();
    void Exit();

    void SetWindowTitle(const std::string& text);

protected:
    virtual void Initialize() { }
    virtual void Cleanup() { }
    virtual void Draw() { }
    virtual void OnKeyDown(int key) { }
    virtual void OnKeyUp(int key) { }
    virtual void OnMouseDown(int button, int x, int y) { }
    virtual void OnMouseMove(int x, int y) { }
    virtual void OnMouseUp(int button, int x, int y) { }
    virtual void OnMouseScroll(float x, float y) { }
    virtual void OnResize(int width, int height) { }
    virtual void Update(float timeElapsed) { }

    VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
    VkDevice GetDevice() const { return m_device; }
    VkFramebuffer GetFramebuffer() { return m_framebuffer.handle; }
    VkImage GetColorAttachment() { return m_framebuffer.colorImage; }
    VkImageView GetColorAttachmentView() { return m_framebuffer.colorImageView; }
    GLFWwindow* GetWindow() const { return m_window; }
    void GetWindowSize(int* width, int* height);

    void CreateFramebuffer();

    typedef std::tuple<std::string, VkShaderStageFlagBits> PipelineShaderInfo;
    VkShaderModule CreateShaderModule(const std::string& filename, const std::string& entryPoint, VkShaderStageFlagBits stage);    
    bool CreatePipeline(const std::vector<PipelineShaderInfo>& pipelineShaderInfo, VkPipeline* pPipeline, std::vector<VkShaderModule>* shaderModules);

private:
    friend void WindowSizeCallback(GLFWwindow* window, int width, int height);
    friend void CursorPosCallback(GLFWwindow* window, double x, double y);
    friend void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    friend void MouseScrollCallback(GLFWwindow* window, double x, double y);
    friend void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    std::string m_name;
    int m_width = 0, m_height = 0;
    int m_physicalDeviceIndex = 0;
    bool m_manageFramebuffer = false;
    VkSampleCountFlagBits m_sampleCountFlag = VK_SAMPLE_COUNT_1_BIT;
    GLFWwindow* m_window = nullptr;
    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkSurface m_surface = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;

    struct {
        VkImage colorImage = VK_NULL_HANDLE;
        VkImageView colorImageView = VK_NULL_HANDLE;
        VkImage depthStencilImage = VK_NULL_HANDLE;
        VkImageView depthStencilImageView = VK_NULL_HANDLE;
        VkFramebuffer handle = VK_NULL_HANDLE;
    } m_framebuffer;

    bool m_quitSignaled = false;
    std::string m_windowTitleText;
};