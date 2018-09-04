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
#include <array>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <stb_image.h>
#include "Window.h"

#define WINDOW_COUNT 4

VkInstance instance = VK_NULL_HANDLE;
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkDevice device = VK_NULL_HANDLE;
std::array<Window*, WINDOW_COUNT> windows;
VkBuffer vertexBuffer = VK_NULL_HANDLE;
VkBuffer indexBuffer = VK_NULL_HANDLE;
VkImage image = VK_NULL_HANDLE;
VkImageView imageView = VK_NULL_HANDLE;
VkSampler sampler = VK_NULL_HANDLE;
VkShaderModule vertShaderModule = VK_NULL_HANDLE;
VkShaderModule fragShaderModule = VK_NULL_HANDLE;
VezPipeline pipeline = VK_NULL_HANDLE;
VkQueue graphicsQueue = VK_NULL_HANDLE;
std::array<VkCommandBuffer, WINDOW_COUNT> commandBuffers;

bool StandardValidationPresent()
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> layerProperties(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());

    bool standardValidationFound = false;
    for (auto prop : layerProperties)
    {
        if (std::string(prop.layerName) == "VK_LAYER_LUNARG_standard_validation")
            return true;
    }

    return false;
}

void InitializeVEZ()
{
    // Get required GLFW layer extensions.
    glfwInit();
    uint32_t instanceExtensionCount = 0U;
    auto instanceExtensions = glfwGetRequiredInstanceExtensions(&instanceExtensionCount);

    // Initialize an instance.
    std::vector<const char*> enabledLayers = {};
    std::vector<const char*> enabledExtensions = {};
    if (StandardValidationPresent())
    {
        enabledLayers.push_back("VK_LAYER_LUNARG_standard_validation");
        enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    for (auto i = 0U; i < instanceExtensionCount; ++i)
        enabledExtensions.push_back(instanceExtensions[i]);

    VezApplicationInfo appInfo = { nullptr, "MultiWindow", VK_MAKE_VERSION(1, 0, 0), "", VK_MAKE_VERSION(0, 0, 0) };
    VezInstanceCreateInfo createInfo = { nullptr, &appInfo, static_cast<uint32_t>(enabledLayers.size()), enabledLayers.data(), static_cast<uint32_t>(enabledExtensions.size()), enabledExtensions.data() };
    auto result = vezCreateInstance(&createInfo, &instance);
    if (result != VK_SUCCESS)
        FATAL("vezCreateInstance failed");

    // Pick the first physical device.
    uint32_t physicalDeviceCount = 0; 
    vezEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vezEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
    physicalDevice = physicalDevices[0];

    // Create the device.
    std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VezDeviceCreateInfo deviceCreateInfo = { nullptr, 0, nullptr, static_cast<uint32_t>(deviceExtensions.size()), deviceExtensions.data() };
    result = vezCreateDevice(physicalDevice, &deviceCreateInfo, &device);
    if (result != VK_SUCCESS)
        FATAL("vezCreateDevice failed");
}

void CreateGeometry()
{
    // A single quad with positions, normals and uvs.
    struct Vertex
    {
        float x, y, z;
        float nx, ny, nz;
        float u, v;
    };

    Vertex vertices[] = {
        { -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
        { 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f },
        { 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
        { -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f }
    };

    // Create the device side vertex buffer.
    VezBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.size = sizeof(vertices);
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    auto result = vezCreateBuffer(device, VEZ_MEMORY_GPU_ONLY, &bufferCreateInfo, &vertexBuffer);
    if (result != VK_SUCCESS)
        FATAL("vezCreateBuffer failed for vertex buffer");

    // Upload the host side data.
    result = vezBufferSubData(device, vertexBuffer, 0, sizeof(vertices), static_cast<void*>(vertices));
    if (result != VK_SUCCESS)
        FATAL("vezBufferSubData failed for vertex buffer");

    // A single quad with positions, normals and uvs.
    uint32_t indices[] = {
        0, 1, 2,
        0, 2, 3,
    };

    // Create the device side index buffer.
    bufferCreateInfo.size = sizeof(indices);
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    result = vezCreateBuffer(device, VEZ_MEMORY_GPU_ONLY, &bufferCreateInfo, &indexBuffer);
    if (result != VK_SUCCESS)
        FATAL("vezCreateBuffer failed for index buffer");

    // Upload the host side data.
    result = vezBufferSubData(device, indexBuffer, 0, sizeof(indices), static_cast<void*>(indices));
    if (result != VK_SUCCESS)
        FATAL("vezBufferSubData failed for index buffer");
}

void CreateTexture()
{
    // Load image from disk.
    int width, height, channels;
    auto pixelData = stbi_load("../../Samples/Data/Textures/texture.jpg", &width, &height, &channels, 4);

    // Create the image.
    VezImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageCreateInfo.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    auto result = vezCreateImage(device, VEZ_MEMORY_GPU_ONLY, &imageCreateInfo, &image);
    if (result != VK_SUCCESS)
        FATAL("vezCreateImage failed");

    // Upload the host side data.
    VezImageSubDataInfo subDataInfo = {};
    subDataInfo.imageSubresource.mipLevel = 0;
    subDataInfo.imageSubresource.baseArrayLayer = 0;
    subDataInfo.imageSubresource.layerCount = 1;
    subDataInfo.imageOffset = { 0, 0, 0 };
    subDataInfo.imageExtent = { imageCreateInfo.extent.width, imageCreateInfo.extent.height, 1 };
    result = vezImageSubData(device, image, &subDataInfo, reinterpret_cast<const void*>(pixelData));
    if (result != VK_SUCCESS)
        FATAL("vezImageSubData failed");

    // Destroy the pixel data.
    stbi_image_free(pixelData);

    // Create the image view for binding the texture as a resource.
    VezImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.image = image;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = imageCreateInfo.format;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    result = vezCreateImageView(device, &imageViewCreateInfo, &imageView);
    if (result != VK_SUCCESS)
        FATAL("vezCreateImageView failed");
    
    // Create the sampler.
    VezSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    result = vezCreateSampler(device, &samplerCreateInfo, &sampler);
    if (result != VK_SUCCESS)
        FATAL("vezCreateSampler failed");
}

VkShaderModule CreateShaderModule(const std::string& filename, const std::string& entryPoint, VkShaderStageFlagBits stage)
{
    // Load the GLSL shader code from disk.
    std::ifstream filestream(filename.c_str(), std::ios::in | std::ios::binary);
    if (!filestream.good())
        FATAL("Failed to open shader file");

    std::string code = std::string((std::istreambuf_iterator<char>(filestream)), std::istreambuf_iterator<char>());
    filestream.close();

    // Create the shader module.
    VezShaderModuleCreateInfo createInfo = {};
    createInfo.stage = stage;
    createInfo.codeSize = static_cast<uint32_t>(code.size());
    if (filename.find(".spv") != std::string::npos)
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.c_str());
    else
        createInfo.pGLSLSource = code.c_str();
    createInfo.pEntryPoint = entryPoint.c_str();

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    auto result = vezCreateShaderModule(device, &createInfo, &shaderModule);
    if (result != VK_SUCCESS && shaderModule != VK_NULL_HANDLE)
    {
        // If shader module creation failed but error is from GLSL compilation, get the error log.
        uint32_t infoLogSize = 0;
        vezGetShaderModuleInfoLog(shaderModule, &infoLogSize, nullptr);

        std::string infoLog(infoLogSize, '\0');
        vezGetShaderModuleInfoLog(shaderModule, &infoLogSize, &infoLog[0]);

        vezDestroyShaderModule(device, shaderModule);
        std::cout << infoLog << "\n";
        FATAL("CreateShaderModule failed");
    }

    return shaderModule;
}

void CreatePipeline()
{
    // Create the shader modules from each stage.
    std::array<VezPipelineShaderStageCreateInfo, 2> shaderStages = { { {}, {} } };
    shaderStages[0].module = CreateShaderModule("../../Samples/Data/Shaders/MultiWindow/Simple.vert", "main", VK_SHADER_STAGE_VERTEX_BIT);    
    shaderStages[1].module = CreateShaderModule("../../Samples/Data/Shaders/MultiWindow/Simple.frag", "main", VK_SHADER_STAGE_FRAGMENT_BIT);    

    // Create the graphics pipeline.
    VezGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();
    auto result = vezCreateGraphicsPipeline(device, &pipelineCreateInfo, &pipeline);
    if (result != VK_SUCCESS)
        FATAL("vezCreateGraphicsPipeline failed");

    // Save shader module handles.
    vertShaderModule = shaderStages[0].module;
    fragShaderModule = shaderStages[1].module;
}

void RecordCommandBuffers()
{
    // One command buffer for each window.
    for (auto i = 0U; i < static_cast<uint32_t>(commandBuffers.size()); ++i)
    {
        // Get the window dimensions.
        uint32_t width, height;
        windows[i]->GetWindowSize(&width, &height);

        // Begin recording.
        auto cmdBuffer = commandBuffers[i];
        vezBeginCommandBuffer(cmdBuffer, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

        // Set the viewport state and dimensions.
        VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
        VkRect2D scissor = { { 0, 0 },{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) } };
        vezCmdSetViewport(0, 1, &viewport);
        vezCmdSetScissor(0, 1, &scissor);
        vezCmdSetViewportState(1);

        // Define clear values for the swapchain's color and depth attachments.
        std::array<VezAttachmentReference, 2> attachmentReferences = {};
        attachmentReferences[0].clearValue.color = { 0.3f, 0.3f, 0.3f, 0.0f };
        attachmentReferences[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentReferences[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentReferences[1].clearValue.depthStencil.depth = 1.0f;
        attachmentReferences[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentReferences[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        // Begin a render pass.
        VezRenderPassBeginInfo beginInfo = {};
        beginInfo.framebuffer = windows[i]->GetFramebuffer();
        beginInfo.attachmentCount = static_cast<uint32_t>(attachmentReferences.size());
        beginInfo.pAttachments = attachmentReferences.data();
        vezCmdBeginRenderPass(&beginInfo);

        // Bind the pipeline and associated resources.
        vezCmdBindPipeline(pipeline);
        vezCmdBindImageView(imageView, sampler, 0, 0, 0);

        // Set push constant buffer with matrix transforms.
        struct {
            glm::mat4 modelViewProj;
        } pushConstants;

        glm::vec3 eyePos = glm::vec3(0.0f, 0.0f, 3.0f);
        if (i == 1) eyePos = glm::vec3(0.0f, 8.0f, 3.0f);        
        float angle = 10.0f * glfwGetTime() * pow(8.0f, i);

        glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 view = glm::lookAt(eyePos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));        
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), width / static_cast<float>(height), 0.1f, 10.0f);
        projection[1][1] *= -1.0f;

        pushConstants.modelViewProj = projection * view * model;
        vezCmdPushConstants(0, sizeof(pushConstants), reinterpret_cast<const void*>(&pushConstants));

        // Bind the vertex buffer and index buffers.
        VkDeviceSize offset = 0;
        vezCmdBindVertexBuffers(0, 1, &vertexBuffer, &offset);
        vezCmdBindIndexBuffer(indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        // Draw the quad.
        vezCmdDrawIndexed(6, 1, 0, 0, 0);

        // End the render pass.
        vezCmdEndRenderPass();

        // End recording.
        vezEndCommandBuffer();
    }
}

int main(int argc, char** argv)
{
    // Initialize the VEZ instance, physical device and device.
    InitializeVEZ();

    // Create spinning quad scene.
    CreateGeometry();
    CreateTexture();

    // Create two windows to render into.
    for (auto& win : windows)
        win = new Window(instance, device, "MultiWindow", 640, 480);
    
    // Create graphics pipeline.
    CreatePipeline();

    // Get the graphics queue.
    vezGetDeviceGraphicsQueue(device, 0, &graphicsQueue);

    // Create separate command buffers for each window.
    VezCommandBufferAllocateInfo allocInfo = {};
    allocInfo.queue = graphicsQueue;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
    auto result = vezAllocateCommandBuffers(device, &allocInfo, commandBuffers.data());
    if (result != VK_SUCCESS)
        FATAL("vezAllocateCommandBuffers failed");

    // Loop until both windows are closed.
    bool windowClosed = false;
    while (!windowClosed)
    {
        // Record command buffers.
        RecordCommandBuffers();

        // Submit command buffers.
        VkSemaphore renderingFinishedSemaphore = VK_NULL_HANDLE;
        VezSubmitInfo submitInfo = {};
        submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
        submitInfo.pCommandBuffers = commandBuffers.data();
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderingFinishedSemaphore;
        vezQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);

        // Present to each window's swapchain.
        std::vector<VezSwapchain> swapchains;
        std::vector<VkImage> srcImages;
        for (auto i = 0U; i < static_cast<uint32_t>(windows.size()); ++i)
        {
            swapchains.push_back(windows[i]->GetSwapchain());
            srcImages.push_back(windows[i]->GetColorAttachment());
        }

        VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VezPresentInfo presentInfo = {};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderingFinishedSemaphore;
        presentInfo.pWaitDstStageMask = &waitDstStageMask;
        presentInfo.swapchainCount = static_cast<uint32_t>(swapchains.size());
        presentInfo.pSwapchains = swapchains.data();
        presentInfo.pImages = srcImages.data();
        vezQueuePresent(graphicsQueue, &presentInfo);

        // Wait for device to be idle so command buffers can be re-recorded.
        vkDeviceWaitIdle(device);

        // Process window events.
        glfwPollEvents();

        // Check to see if any window was closed.
        for (auto win : windows)
            windowClosed |= win->IsClosed();
    };

    // Destroy resouces.
    vezFreeCommandBuffers(device, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
    vezDestroyPipeline(device, pipeline);
    vezDestroyShaderModule(device, fragShaderModule);
    vezDestroyShaderModule(device, vertShaderModule);
    vezDestroySampler(device, sampler);
    vezDestroyImageView(device, imageView);
    vezDestroyImage(device, image);
    vezDestroyBuffer(device, indexBuffer);
    vezDestroyBuffer(device, vertexBuffer);

    for (auto win : windows)
        delete win;

    vezDestroyDevice(device);
    vezDestroyInstance(instance);

    return 0;
}