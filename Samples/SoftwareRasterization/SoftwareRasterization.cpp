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
#include <chrono>
#include <cmath>
#include <stb_image.h>
#include "SoftwareRasterization.h"

#define FATAL(msg) { std::cout << msg << "\n"; AppBase::Exit(); return; }

struct UniformBuffer
{
    glm::mat4 view;
    glm::mat4 projection;
    uint32_t width, height;
};

SoftwareRasterization::SoftwareRasterization()
    : AppBase("SoftwareRasterization Sample", 800, 600, 0, true, {}, false)
{

}

void SoftwareRasterization::Initialize()
{
    CreateColorOutput();
    CreateTexture();
    CreateUniformBuffer();
    CreatePipeline();
    CreateCommandBuffer();
}

void SoftwareRasterization::Cleanup()
{    
    auto device = AppBase::GetDevice();

    vezDestroyImageView(device, m_colorOutput.imageView);
    vezDestroyImage(device, m_colorOutput.image);
    
    vezDestroyImageView(device, m_imageView);
    vezDestroyImage(device, m_image);
    vezDestroySampler(device, m_sampler);

    vezDestroyBuffer(device, m_uniformBuffer);

    vezDestroyPipeline(device, m_computePipeline.pipeline);
    for (auto shaderModule : m_computePipeline.shaderModules)
        vezDestroyShaderModule(device, shaderModule);
    
    vezFreeCommandBuffers(device, 1, &m_commandBuffer);
}

void SoftwareRasterization::Draw()
{
    // Submit the command buffer to the graphics queue.
    VezSubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffer;

    // Request a wait semaphore to pass to present so it waits for rendering to complete.
    VkSemaphore semaphore = VK_NULL_HANDLE;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphore;
    if (vezQueueSubmit(m_graphicsQueue, 1, &submitInfo, nullptr) != VK_SUCCESS)
        FATAL("vkQueueSubmit failed");

    // Present the swapchain framebuffer to the window.
    VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    auto swapchain = AppBase::GetSwapchain();
    auto srcImage = AppBase::GetColorAttachment();

    VezPresentInfo presentInfo = {};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &semaphore;
    presentInfo.pWaitDstStageMask = &waitDstStageMask;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImages = &m_colorOutput.image;
    if (vezQueuePresent(m_graphicsQueue, &presentInfo) != VK_SUCCESS)
        FATAL("vezQueuePresentKHR failed");
}

void SoftwareRasterization::OnResize(int width, int height)
{
    // Recreate color output.
    CreateColorOutput();

    // Recreate command buffer.
    vezFreeCommandBuffers(AppBase::GetDevice(), 1, &m_commandBuffer);
    CreateCommandBuffer();
}

void SoftwareRasterization::Update(float timeElapsed)
{
    // Get the current window dimensions.
    int width, height;
    AppBase::GetWindowSize(&width, &height);

    // Calculate elapsed time.
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - startTime).count() / 1000.0f;

    // Calculate appropriate matrices for the current frame.
    UniformBuffer ub = {};
    ub.view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.5f)) * glm::rotate(glm::mat4(1.0f), sin(elapsedTime * 0.001f) * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    ub.projection = glm::perspective(glm::radians(65.0f), width / static_cast<float>(height), 0.1f, 10.0f);
    ub.projection[1][1] *= -1.0f;
    ub.width = static_cast<uint32_t>(width);
    ub.height = static_cast<uint32_t>(height);

    // Update the memory via map and unmap since it was created as HOST_VISIBLE (i.e. VK_MEMORY_USAGE_CPU_TO_GPU).
    void* data = nullptr;
    auto result = vezMapBuffer(AppBase::GetDevice(), m_uniformBuffer, 0, sizeof(UniformBuffer), &data);
    if (result != VK_SUCCESS)
        FATAL("vkMapBuffer failed");

    memcpy(data, &ub, sizeof(UniformBuffer));
    vezUnmapBuffer(AppBase::GetDevice(), m_uniformBuffer);
}

void SoftwareRasterization::CreateColorOutput()
{
    // Free previous allocations.
    if (m_colorOutput.imageView)
    {
        vezDestroyImageView(AppBase::GetDevice(), m_colorOutput.imageView);
        vezDestroyImage(AppBase::GetDevice(), m_colorOutput.image);
    }

    // Get the current window dimension.
    int width, height;
    AppBase::GetWindowSize(&width, &height);

    // Match the swapchain's image format.
    VkSurfaceFormatKHR swapchainFormat = {};
    vezGetSwapchainSurfaceFormat(AppBase::GetSwapchain(), &swapchainFormat);

    // Create the color image for the m_framebuffer.
    VezImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = swapchainFormat.format;
    imageCreateInfo.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    auto result = vezCreateImage(AppBase::GetDevice(), VEZ_MEMORY_GPU_ONLY, &imageCreateInfo, &m_colorOutput.image);
    if (result != VK_SUCCESS)
        FATAL("vkCreateImage failed");

    // Create the image view for binding the texture as a resource.
    VezImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.image = m_colorOutput.image;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = imageCreateInfo.format;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    result = vezCreateImageView(AppBase::GetDevice(), &imageViewCreateInfo, &m_colorOutput.imageView);
    if (result != VK_SUCCESS)
        FATAL("vkCreateImageView failed");
}

void SoftwareRasterization::CreateTexture()
{
    // Load image from disk.
    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    auto pixelData = stbi_load("../../Samples/Data/Textures/texture.jpg", &width, &height, &channels, 4);

    // Create the AppBase::GetDevice() side image.
    VezImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageCreateInfo.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    auto result = vezCreateImage(AppBase::GetDevice(), VEZ_MEMORY_GPU_ONLY, &imageCreateInfo, &m_image);
    if (result != VK_SUCCESS)
        FATAL("vezCreateImage failed");

    // Upload the host side data.
    VezImageSubDataInfo subDataInfo = {};
    subDataInfo.imageSubresource.mipLevel = 0;
    subDataInfo.imageSubresource.baseArrayLayer = 0;
    subDataInfo.imageSubresource.layerCount = 1;
    subDataInfo.imageOffset = { 0, 0, 0 };
    subDataInfo.imageExtent = { imageCreateInfo.extent.width, imageCreateInfo.extent.height, 1 };
    result = vezImageSubData(AppBase::GetDevice(), m_image, &subDataInfo, reinterpret_cast<const void*>(pixelData));
    if (result != VK_SUCCESS)
        FATAL("vezImageSubData failed");

    // Destroy the pixel data.
    stbi_image_free(pixelData);

    // Create the image view for binding the texture as a resource.
    VezImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.image = m_image;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = imageCreateInfo.format;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    result = vezCreateImageView(AppBase::GetDevice(), &imageViewCreateInfo, &m_imageView);
    if (result != VK_SUCCESS)
        FATAL("vezCreateImageView failed");

    VezSamplerCreateInfo createInfo = {};
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    result = vezCreateSampler(AppBase::GetDevice(), &createInfo, &m_sampler);
    if (result != VK_SUCCESS)
        FATAL("vezCreateSampler failed");
}

void SoftwareRasterization::CreateUniformBuffer()
{
    // Create a buffer for storing per frame matrices.
    VezBufferCreateInfo createInfo = {};
    createInfo.size = sizeof(UniformBuffer);
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (vezCreateBuffer(AppBase::GetDevice(), VEZ_MEMORY_CPU_TO_GPU, &createInfo, &m_uniformBuffer) != VK_SUCCESS)
        FATAL("vkCreateBuffer failed for uniform buffer");
}

void SoftwareRasterization::CreatePipeline()
{
    // Create the graphics pipeline.
    if (!AppBase::CreatePipeline(
        { { "../../Samples/Data/Shaders/SoftwareRasterization/SoftwareRasterization.comp", VK_SHADER_STAGE_COMPUTE_BIT } },
        &m_computePipeline.pipeline, &m_computePipeline.shaderModules))
    {
        AppBase::Quit();
    }
}

void SoftwareRasterization::CreateCommandBuffer()
{
    // Get the graphics queue handle.
    vezGetDeviceGraphicsQueue(AppBase::GetDevice(), 0, &m_graphicsQueue);

    // Create a command buffer handle.
    VezCommandBufferAllocateInfo allocInfo = {};
    allocInfo.queue = m_graphicsQueue;
    allocInfo.commandBufferCount = 1;
    if (vezAllocateCommandBuffers(AppBase::GetDevice(), &allocInfo, &m_commandBuffer) != VK_SUCCESS)
        FATAL("vkAllocateCommandBuffers failed");

    // Begin command buffer recording.
    if (vezBeginCommandBuffer(m_commandBuffer, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT) != VK_SUCCESS)
        FATAL("vkBeginCommandBuffer failed");

    // Get the current window dimensions.
    int width, height;
    AppBase::GetWindowSize(&width, &height);
    
    // Clear the color attachment.
    VkClearColorValue clearColor = {};
    VezImageSubresourceRange range = {};
    range.layerCount = 1;
    range.levelCount = 1;
    vezCmdClearColorImage(m_colorOutput.image, &clearColor, 1, &range);

    // Bind the compute pipeline and resouces.
    vezCmdBindPipeline(m_computePipeline.pipeline);
    vezCmdBindBuffer(m_uniformBuffer, 0, VK_WHOLE_SIZE, 0, 0, 0);
    vezCmdBindImageView(m_imageView, m_sampler, 0, 1, 0);
    vezCmdBindImageView(m_colorOutput.imageView, VK_NULL_HANDLE, 0, 2, 0);

    // Dispatch compute to rasterize N simulated primitives.
    auto groupCountX = static_cast<uint32_t>(ceilf(width / 8.0f));
    auto groupCountY = static_cast<uint32_t>(ceilf(height / 8.0f));
    vezCmdDispatch(groupCountX, groupCountY, 1U);

    // End command buffer recording.
    if (vezEndCommandBuffer() != VK_SUCCESS)
        FATAL("vkEndCommandBuffer failed");
}