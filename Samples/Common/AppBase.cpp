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
#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <map>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <VEZ.h>
#include "AppBase.h"

static std::map<GLFWwindow*, AppBase*> appBaseInstances;

static void SetWindowCenter(GLFWwindow* window)
{
    // Get window position and size
    int posX, posY;
    glfwGetWindowPos(window, &posX, &posY);

    int width, height;
    glfwGetWindowSize(window, &width, &height);

    // Halve the window size and use it to adjust the window position to the center of the window
    width >>= 1;
    height >>= 1;

    posX += width;
    posY += height;

    // Get the list of monitors
    int count;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    if (monitors == nullptr)
        return;

    // Figure out which monitor the window is in
    GLFWmonitor *owner = NULL;
    int owner_x, owner_y, owner_width, owner_height;

    for (int i = 0; i < count; i++)
    {
        // Get the monitor position
        int monitor_x, monitor_y;
        glfwGetMonitorPos(monitors[i], &monitor_x, &monitor_y);

        // Get the monitor size from its video mode
        int monitor_width, monitor_height;
        GLFWvidmode *monitor_vidmode = (GLFWvidmode*)glfwGetVideoMode(monitors[i]);

        if (monitor_vidmode == NULL)
            continue;
        
        monitor_width = monitor_vidmode->width;
        monitor_height = monitor_vidmode->height;

        // Set the owner to this monitor if the center of the window is within its bounding box
        if ((posX > monitor_x && posX < (monitor_x + monitor_width)) && (posY > monitor_y && posY < (monitor_y + monitor_height)))
        {
            owner = monitors[i];
            owner_x = monitor_x;
            owner_y = monitor_y;
            owner_width = monitor_width;
            owner_height = monitor_height;
        }
    }

    // Set the window position to the center of the owner monitor
    if (owner != NULL)
        glfwSetWindowPos(window, owner_x + (owner_width >> 1) - width, owner_y + (owner_height >> 1) - height);
}

static void WindowSizeCallback(GLFWwindow* window, int width, int height)
{
    auto itr = appBaseInstances.find(window);
    if (itr != appBaseInstances.end())
    {
        // Wait for device to be idle.
        vkDeviceWaitIdle(itr->second->GetDevice());

        // Re-create the framebuffer.
        if (itr->second->m_manageFramebuffer)
            itr->second->CreateFramebuffer();

        // Now inform application of resize event.
        itr->second->OnResize(width, height);
    }
}

static void CursorPosCallback(GLFWwindow* window, double x, double y)
{
    auto itr = appBaseInstances.find(window);
    if (itr != appBaseInstances.end())
    {
        itr->second->OnMouseMove(static_cast<int>(x), static_cast<int>(y));
    }
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    auto itr = appBaseInstances.find(window);
    if (itr != appBaseInstances.end())
    {
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        if (action == GLFW_PRESS)
        {
            itr->second->OnMouseDown(button, static_cast<int>(x), static_cast<int>(y));
        }
        else if (action == GLFW_RELEASE)
        {
            itr->second->OnMouseUp(button, static_cast<int>(x), static_cast<int>(y));
        }
    }
}

static void MouseScrollCallback(GLFWwindow* window, double x, double y)
{
    auto itr = appBaseInstances.find(window);
    if (itr != appBaseInstances.end())
    {
        itr->second->OnMouseScroll(static_cast<float>(x), static_cast<float>(y));
    }
}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto itr = appBaseInstances.find(window);
    if (itr != appBaseInstances.end())
    {
        if (action == GLFW_PRESS)
        {
            itr->second->OnKeyDown(key);
        }
        else if (action == GLFW_RELEASE)
        {
            itr->second->OnKeyUp(key);
        }
    }
}

AppBase::AppBase(const char* name, int width, int height, int physicalDeviceIndex, bool manageFramebuffer, VkSampleCountFlagBits sampleCountFlag)
    : m_name(name)
    , m_width(width)
    , m_height(height)
    , m_physicalDeviceIndex(physicalDeviceIndex)
    , m_manageFramebuffer(manageFramebuffer)
    , m_sampleCountFlag(sampleCountFlag)
{
    
}

AppBase::~AppBase()
{
    if (m_window)
    {
        // Remove AppBase and GLFWwindow pair from static map.
        appBaseInstances.erase(m_window);

        // Destroy the GLFW window.
        glfwDestroyWindow(m_window);

        // Terminate the GLFW library.
        glfwTerminate();
    }
}

int AppBase::Run()
{
#ifdef VKEZ_NO_PROTOTYPES
    // Initialize VulkanEZ library.
    if (InitVulkanEZ() != VK_SUCCESS)
    {
        std::cout << "InitVulkanEZ failed\n";
        return -1;
    }
#endif

    // Initialize a VulkanEZ instance and enable the standard set of LunarG validation layers.
#ifdef _DEBUG
    std::vector<const char*> enabledLayers = { "VK_LAYER_LUNARG_standard_validation" };
    std::vector<const char*> enabledExtensions = { VK_EXT_DEBUG_REPORT_EXTENSION_NAME  };
#else
    std::vector<const char*> enabledLayers = { };
    std::vector<const char*> enabledExtensions = {};
#endif
    VkApplicationInfo appInfo = { m_name.c_str(), VEZ_MAKE_VERSION(1, 0, 0), "", VEZ_MAKE_VERSION(0, 0, 0) };
    VkInstanceCreateInfo createInfo = {
        &appInfo,
        static_cast<uint32_t>(enabledLayers.size()), enabledLayers.data(),
        static_cast<uint32_t>(enabledExtensions.size()), enabledExtensions.data()
    };
    auto result = vkCreateInstance(&createInfo, &m_instance);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkCreateInstance failed result=" << result << "\n";
        return -1;
    }

    // Enumerate all attached physical devices.
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);
    if (physicalDeviceCount == 0)
    {
        std::cout << "No vulkan physical devices found\n";
        return -1;
    }

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data());

    // Select the physical device.
    m_physicalDevice = physicalDevices[m_physicalDeviceIndex];

    // Display physical device properties to the console.
    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);
    
    auto deviceTypeToString = [](VkPhysicalDeviceType type) -> std::string {
        switch (type)
        {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER: return "OTHER";
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "INTEGRATED_GPU";
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return "DISCRETE_GPU";
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return "VIRTUAL_GPU";
        case VK_PHYSICAL_DEVICE_TYPE_CPU: return "CPU";
        default: return "UNKNOWN";
        }
    };

    std::cout << "Name: " << properties.deviceName << "\n";
    std::cout << "Type: " << deviceTypeToString(properties.deviceType) << "\n";

    // Initialize a window using GLFW and hint no graphics API should be used on the backend.
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(m_width, m_height, m_name.c_str(), nullptr, nullptr);
    SetWindowCenter(m_window);

    // Create a surface from the GLFW window handle.
    VkSurfaceCreateInfo surfaceCreateInfo = {};
    surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);
    surfaceCreateInfo.hwnd = glfwGetWin32Window(m_window);
    result = vkCreateSurface(m_instance, &surfaceCreateInfo, &m_surface);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkCreateSurface failed!\n";
        return -1;
    }

    // Specify swapchain surface properties.
    VkSwapchainCreateInfo swapchainCreateInfo = {};
    swapchainCreateInfo.surface = m_surface;
    swapchainCreateInfo.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    // Create the Vulkan device handle (no extensions enabled).
    std::array<const char*, 1> deviceExtensions = {
        "VK_EXT_shader_subgroup_ballot"
    };

    VkDeviceCreateInfo deviceCreateInfo = { static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data(), &swapchainCreateInfo };
    result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, &m_device);
    if (result != VK_SUCCESS)
    {
        std::cout << "vkCreateDevice failed\n";
        return -1;
    }

    // Store AppBase and GLFWwindow pair in static map.
    appBaseInstances[m_window] = this;
    
    // Set callbacks.
    glfwSetWindowSizeCallback(m_window, WindowSizeCallback);
    glfwSetCursorPosCallback(m_window, CursorPosCallback);
    glfwSetMouseButtonCallback(m_window, MouseButtonCallback);
    glfwSetScrollCallback(m_window, MouseScrollCallback);
    glfwSetKeyCallback(m_window, KeyCallback);

    // Display the window.
    glfwShowWindow(m_window);

    // Create framebuffer.
    if (m_manageFramebuffer)
        CreateFramebuffer();

    // Initialize the application.
    Initialize();  

    // Track time elapsed from one Update call to the next.
    double lastTime = glfwGetTime();

    // Track fps.
    double elapsedTime = 0.0;
    uint32_t frameCount = 0;

    // Message loop.
    while (!glfwWindowShouldClose(m_window) && !m_quitSignaled)
    {
        // Check for window messages to process.
        glfwPollEvents();

        // Update the application.
        double curTime = glfwGetTime();
        elapsedTime += curTime - lastTime;
        Update(static_cast<float>(curTime - lastTime));
        lastTime = curTime;

        // Draw the application.
        Draw();

        // Display the fps in the window title bar.
        ++frameCount;
        if (elapsedTime >= 1.0)
        {
            std::string text = m_name + " " + std::to_string(frameCount) + " FPS";
            if (m_windowTitleText.size() > 0)
                text += "    (" + m_windowTitleText + ")";
            glfwSetWindowTitle(m_window, text.c_str());
            elapsedTime = 0.0;
            frameCount = 0;
        }
    }

    // Wait for all device operations to complete.
    vkDeviceWaitIdle(m_device);
    
    // Call application's Cleanup method.
    Cleanup();
    
    // Destroy framebuffer.
    vkDestroyImageView(m_device, m_framebuffer.depthStencilImageView);
    vkDestroyImageView(m_device, m_framebuffer.colorImageView);
    vkDestroyImage(m_device, m_framebuffer.depthStencilImage);
    vkDestroyImage(m_device, m_framebuffer.colorImage);
    vkDestroyFramebuffer(m_device, m_framebuffer.handle);

    // Destroy device.
    vkDestroyDevice(m_device);

    // Destroy surface.
    vkDestroySurface(m_instance, m_surface);

    // Destroy instance.
    vkDestroyInstance(m_instance);

    // Return success.
    return 0;
}

void AppBase::Quit()
{
    m_quitSignaled = true;
}

void AppBase::Exit()
{
    // Wait for all device operations to complete.
    vkDeviceWaitIdle(m_device);

    // Call application's Cleanup method.
    Cleanup();

    // Destroy framebuffer.
    if (m_manageFramebuffer)
    {
        if (m_framebuffer.handle)
        {
            vkDestroyFramebuffer(m_device, m_framebuffer.handle);
            vkDestroyImageView(m_device, m_framebuffer.colorImageView);
            vkDestroyImageView(m_device, m_framebuffer.depthStencilImageView);
            vkDestroyImage(m_device, m_framebuffer.colorImage);
            vkDestroyImage(m_device, m_framebuffer.depthStencilImage);
        }
    }

    // Destroy device.
    vkDestroyDevice(m_device);

    // Force exit the process.
    exit(-1);
}

void AppBase::SetWindowTitle(const std::string& text)
{
    m_windowTitleText = text;
}

void AppBase::GetWindowSize(int* width, int* height)
{
    if (m_window)
        glfwGetWindowSize(m_window, width, height);
}

void AppBase::CreateFramebuffer()
{
    // Free previous allocations.
    if (m_framebuffer.handle)
    {
        vkDestroyFramebuffer(m_device, m_framebuffer.handle);
        vkDestroyImageView(m_device, m_framebuffer.colorImageView);
        vkDestroyImageView(m_device, m_framebuffer.depthStencilImageView);
        vkDestroyImage(m_device, m_framebuffer.colorImage);
        vkDestroyImage(m_device, m_framebuffer.depthStencilImage);
    }

    // Get the current window dimension.
    int width, height;
    glfwGetWindowSize(m_window, &width, &height);

    // Create the color image for the m_framebuffer.
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageCreateInfo.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = m_sampleCountFlag;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    auto result = vkCreateImage(m_device, VK_MEMORY_GPU_ONLY, &imageCreateInfo, &m_framebuffer.colorImage);
    if (result != VK_SUCCESS)
    {
        std::cout << __FUNCTION__ << " vkCreateImage failed (" << result << ")\n";
        return;
    }

    // Create the image view for binding the texture as a resource.
    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.image = m_framebuffer.colorImage;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = imageCreateInfo.format;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    result = vkCreateImageView(m_device, &imageViewCreateInfo, &m_framebuffer.colorImageView);
    if (result != VK_SUCCESS)
    {
        std::cout << __FUNCTION__ << " vkCreateImageView failed (" << result << ")\n";
        return;
    }

    // Create the depth image for the m_framebuffer.
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_D32_SFLOAT;
    imageCreateInfo.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = m_sampleCountFlag;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    result = vkCreateImage(m_device, VK_MEMORY_GPU_ONLY, &imageCreateInfo, &m_framebuffer.depthStencilImage);
    if (result != VK_SUCCESS)
    {
        std::cout << __FUNCTION__ << " vkCreateImage failed (" << result << ")\n";
        return;
    }

    // Create the image view for binding the texture as a resource.
    imageViewCreateInfo.image = m_framebuffer.depthStencilImage;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = imageCreateInfo.format;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    result = vkCreateImageView(m_device, &imageViewCreateInfo, &m_framebuffer.depthStencilImageView);
    if (result != VK_SUCCESS)
    {
        std::cout << __FUNCTION__ << " vkCreateImageView failed (" << result << ")\n";
        return;
    }

    // Create the m_framebuffer.
    std::array<VkImageView, 2> attachments = { m_framebuffer.colorImageView, m_framebuffer.depthStencilImageView };
    VkFramebufferCreateInfo framebufferCreateInfo = {};
    framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferCreateInfo.pAttachments = attachments.data();
    framebufferCreateInfo.width = width;
    framebufferCreateInfo.height = height;
    framebufferCreateInfo.layers = 1;
    result = vkCreateFramebuffer(m_device, &framebufferCreateInfo, &m_framebuffer.handle);
    if (result != VK_SUCCESS)
    {
        std::cout << __FUNCTION__ << " vkCreateFramebuffer failed (" << result << ")\n";
        return;
    }
}

VkShaderModule AppBase::CreateShaderModule(const std::string& filename, const std::string& entryPoint, VkShaderStageFlagBits stage)
{
    // Load the GLSL shader code from disk.
    std::ifstream filestream(filename.c_str());
    if (!filestream.good())
    {
        std::cout << "Failed to open " << filename << "\n";
        return VK_NULL_HANDLE;
    }

    std::string code = std::string((std::istreambuf_iterator<char>(filestream)), std::istreambuf_iterator<char>());
    filestream.close();

    // Create the shader module.
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.stage = stage;
    createInfo.codeSize = static_cast<uint32_t>(code.size());
    createInfo.pGLSLSource = code.c_str();
    createInfo.pEntryPoint = entryPoint.c_str();

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    auto result = vkCreateShaderModule(m_device, &createInfo, &shaderModule);
    if (result != VK_SUCCESS)
    {
        // If shader module creation failed but error is from GLSL compilation, get the error log.
        if (result == VK_ERROR_GLSL_COMPILE_FAILED)
        {
            uint32_t infoLogSize = 0;
            vkGetShaderModuleInfoLog(shaderModule, &infoLogSize, nullptr);

            std::string infoLog(infoLogSize, '\0');
            vkGetShaderModuleInfoLog(shaderModule, &infoLogSize, &infoLog[0]);

            vkDestroyShaderModule(m_device, shaderModule);

            std::cout << infoLog << "\n";
            return VK_NULL_HANDLE;
        }

        return VK_NULL_HANDLE;
    }

    return shaderModule;
}

bool AppBase::CreatePipeline(const std::vector<PipelineShaderInfo>& pipelineShaderInfo, VkPipeline* pPipeline, std::vector<VkShaderModule>* shaderModules)
{
    // Create shader modules.
    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfo(pipelineShaderInfo.size());
    for (auto i = 0U; i < pipelineShaderInfo.size(); ++i)
    {
        auto filename = std::get<0>(pipelineShaderInfo[i]);
        auto stage = std::get<1>(pipelineShaderInfo[i]);
        auto shaderModule = CreateShaderModule(filename, "main", stage);
        if (shaderModule == VK_NULL_HANDLE)
        {
            std::cout << "CreateShaderModule failed\n";
            return false;
        }

        shaderStageCreateInfo[i].module = shaderModule;
        shaderStageCreateInfo[i].pEntryPoint = "main";
        shaderModules->push_back(shaderModule);
    }

    // Determine if this is a compute only pipeline.
    bool isComputePipeline = (pipelineShaderInfo.size() == 1 && std::get<1>(pipelineShaderInfo[0]) == VK_SHADER_STAGE_COMPUTE_BIT);

    // Create the graphics pipeline or compute pipeline.
    if (isComputePipeline)
    {
        VkComputePipelineCreateInfo pipelineCreateInfo = {};
        pipelineCreateInfo.pStage = shaderStageCreateInfo.data();
        if (vkCreateComputePipeline(m_device, &pipelineCreateInfo, pPipeline) != VK_SUCCESS)
        {
            std::cout << "vkCreateComputePipeline failed\n";
            return false;
        }
    }
    else
    {
        VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
        pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStageCreateInfo.size());
        pipelineCreateInfo.pStages = shaderStageCreateInfo.data();
        if (vkCreateGraphicsPipeline(m_device, &pipelineCreateInfo, pPipeline) != VK_SUCCESS)
        {
            std::cout << "vkCreateGraphicsPipeline failed\n";
            return false;
        }
    }

    // Return success.
    return true;
}