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
#include <chrono>
#include "Subpasses.h"
#include <FreeImage.h>

#define FATAL(msg) { std::cout << msg << "\n"; AppBase::Exit(); return; }

struct Vertex
{
    float x, y, z;
    float nx, ny, nz;
    float u, v;
};

struct UniformBuffer
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};

Subpasses::Subpasses()
    : AppBase("Subpasses Sample", 500, 400, 0)
{

}

void Subpasses::Initialize()
{
    CreateQuad();
    CreateTexture();
    CreateSampler();
    CreateUniformBuffer();
    CreateFramebuffer();
    CreatePipeline();
    CreateCommandBuffer();
}

void Subpasses::Cleanup()
{    
    auto device = AppBase::GetDevice();

    vkDestroyFramebuffer(AppBase::GetDevice(), m_framebuffer.handle);
    for (auto attachment : m_framebuffer.attachments)
    {
        vkDestroyImageView(AppBase::GetDevice(), attachment.imageView);
        vkDestroyImage(AppBase::GetDevice(), attachment.image);
    }

    vkDestroyBuffer(device, m_vertexBuffer);
    vkDestroyBuffer(device, m_indexBuffer);
    vkDestroyImageView(device, m_imageView);
    vkDestroyImage(device, m_image);
    vkDestroySampler(device, m_sampler);
    vkDestroyBuffer(device, m_uniformBuffer);

    vkDestroyPipeline(device, m_subPass0Pipeline.pipeline);
    for (auto shaderModule : m_subPass0Pipeline.shaderModules)
        vkDestroyShaderModule(device, shaderModule);
    
    vkDestroyPipeline(device, m_subPass1Pipeline.pipeline);
    for (auto shaderModule : m_subPass1Pipeline.shaderModules)
        vkDestroyShaderModule(device, shaderModule);

    vkFreeCommandBuffers(device, 1, &m_commandBuffer);
}

void Subpasses::Draw()
{
    // Submit the command buffer to the graphics queue.
    VkSubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffer;

    // Request a wait semaphore to pass to present so it waits for rendering to complete.
    VkSemaphore semaphore = VK_NULL_HANDLE;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphore;
    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, nullptr) != VK_SUCCESS)
        FATAL("vkQueueSubmit failed");

    // Present the swapchain framebuffer to the window.
    VkPresentInfo presentInfo = { m_framebuffer.attachments[0].image, 1, &semaphore, 0, nullptr };
    if (vkQueuePresent(m_graphicsQueue, &presentInfo) != VK_SUCCESS)
        FATAL("vkQueuePresentKHR failed");

    // Destroy the semaphore.
    vkDestroySemaphore(AppBase::GetDevice(), semaphore);
}

void Subpasses::OnResize(int width, int height)
{
    // Re-create framebuffer.
    CreateFramebuffer();

    // Re-create command buffer.
    vkFreeCommandBuffers(AppBase::GetDevice(), 1, &m_commandBuffer);
    CreateCommandBuffer();
}

void Subpasses::Update(float timeElapsed)
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
    ub.model = glm::rotate(elapsedTime * glm::radians(0.01f), glm::vec3(0.0f, 0.0f, 1.0f));
    ub.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ub.projection = glm::perspective(glm::radians(45.0f), width / static_cast<float>(height), 0.1f, 10.0f);
    ub.projection[1][1] *= -1.0f;

    // Update the memory via map and unmap since it was created as HOST_VISIBLE (i.e. VK_MEMORY_USAGE_CPU_TO_GPU).
    void* data = nullptr;
    auto result = vkMapBuffer(AppBase::GetDevice(), m_uniformBuffer, 0, sizeof(UniformBuffer), &data);
    if (result != VK_SUCCESS)
        FATAL("vkMapBuffer failed");

    memcpy(data, &ub, sizeof(UniformBuffer));
    vkUnmapBuffer(AppBase::GetDevice(), m_uniformBuffer);
}

void Subpasses::CreateQuad()
{
    // A single quad with positions, normals and uvs.
    Vertex vertices[] = {
        { -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
    { 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f },
    { 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f },
    { -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f }
    };

    // Create the device side vertex buffer.
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.size = sizeof(vertices);
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    auto result = vkCreateBuffer(AppBase::GetDevice(), VK_MEMORY_GPU_ONLY, &bufferCreateInfo, &m_vertexBuffer);
    if (result != VK_SUCCESS)
        FATAL("vkCreateBuffer failed for vertex buffer");

    // Upload the host side data.
    result = vkBufferSubData(AppBase::GetDevice(), m_vertexBuffer, 0, sizeof(vertices), static_cast<void*>(vertices));
    if (result != VK_SUCCESS)
        FATAL("vkBufferSubData failed for vertex buffer");

    // A single quad with positions, normals and uvs.
    uint32_t indices[] = {
        0, 1, 2,
        0, 2, 3,
    };

    // Create the device side index buffer.
    bufferCreateInfo.size = sizeof(indices);
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    result = vkCreateBuffer(AppBase::GetDevice(), VK_MEMORY_GPU_ONLY, &bufferCreateInfo, &m_indexBuffer);
    if (result != VK_SUCCESS)
        FATAL("vkCreateBuffer failed for index buffer");

    // Upload the host side data.
    result = vkBufferSubData(AppBase::GetDevice(), m_indexBuffer, 0, sizeof(indices), static_cast<void*>(indices));
    if (result != VK_SUCCESS)
        FATAL("vkBufferSubData failed for index buffer");
}

void Subpasses::CreateTexture()
{
    // Load image from disk.
    FreeImage_Initialise();
    FIBITMAP* bitmap = FreeImage_Load(FIF_JPEG, "../../Samples/Data/Textures/texture.jpg");
    if (!bitmap)
        FATAL("Failed to load texture.jpg");

    // Ensure format is 8-bit RGBA.
    FIBITMAP* temp = FreeImage_ConvertTo32Bits(bitmap);
    FreeImage_Unload(bitmap);
    bitmap = temp;

    // Create the AppBase::GetDevice() side image.
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageCreateInfo.extent = { FreeImage_GetWidth(bitmap), FreeImage_GetHeight(bitmap), 1 };
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    auto result = vkCreateImage(AppBase::GetDevice(), VK_MEMORY_GPU_ONLY, &imageCreateInfo, &m_image);
    if (result != VK_SUCCESS)
        FATAL("vkCreateImage failed");

    // Upload the host side data.
    VkImageSubDataInfo subDataInfo = {};
    subDataInfo.imageSubresource.mipLevel = 0;
    subDataInfo.imageSubresource.baseArrayLayer = 0;
    subDataInfo.imageSubresource.layerCount = 1;
    subDataInfo.imageOffset = { 0, 0, 0 };
    subDataInfo.imageExtent = { FreeImage_GetWidth(bitmap), FreeImage_GetHeight(bitmap), 1 };
    result = vkImageSubData(AppBase::GetDevice(), m_image, &subDataInfo, FreeImage_GetBits(bitmap));
    if (result != VK_SUCCESS)
        FATAL("vkImageSubData failed");

    // Destroy the FreeImage handle.
    FreeImage_Unload(bitmap);
    FreeImage_DeInitialise();

    // Create the image view for binding the texture as a resource.
    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.image = m_image;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = imageCreateInfo.format;
    imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_A };
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    result = vkCreateImageView(AppBase::GetDevice(), &imageViewCreateInfo, &m_imageView);
    if (result != VK_SUCCESS)
        FATAL("vkCreateImageView failed");
}

void Subpasses::CreateSampler()
{
    VkSamplerCreateInfo createInfo = {};
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    auto result = vkCreateSampler(AppBase::GetDevice(), &createInfo, &m_sampler);
    if (result != VK_SUCCESS)
        FATAL("vkCreateSampler failed");
}

void Subpasses::CreateUniformBuffer()
{
    // Create a buffer for storing per frame matrices.
    VkBufferCreateInfo createInfo = {};
    createInfo.size = sizeof(UniformBuffer);
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (vkCreateBuffer(AppBase::GetDevice(), VK_MEMORY_CPU_TO_GPU, &createInfo, &m_uniformBuffer) != VK_SUCCESS)
        FATAL("vkCreateBuffer failed for uniform buffer");
}

void Subpasses::CreateFramebuffer()
{
    // Free previous allocations.
    if (m_framebuffer.handle)
    {
        vkDestroyFramebuffer(AppBase::GetDevice(), m_framebuffer.handle);
        for (auto attachment : m_framebuffer.attachments)
        {
            vkDestroyImageView(AppBase::GetDevice(), attachment.imageView);
            vkDestroyImage(AppBase::GetDevice(), attachment.image);
        }
    }

    // Get the current window dimension.
    int width, height;
    AppBase::GetWindowSize(&width, &height);

    for (auto i = 0U; i < m_framebuffer.attachments.size(); ++i)
    {
        // The very last attachment is the depth stencil format.
        auto& attachment = m_framebuffer.attachments[i];
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        if (i == m_framebuffer.attachments.size() - 1)
        {
            format = VK_FORMAT_D32_SFLOAT;
            usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        }

        // Create the color image for the m_framebuffer.
        VkImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = format;
        imageCreateInfo.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = usage;
        auto result = vkCreateImage(AppBase::GetDevice(), VK_MEMORY_GPU_ONLY, &imageCreateInfo, &attachment.image);
        if (result != VK_SUCCESS)
            FATAL("vkCreateImage failed");

        // Create the image view for binding the texture as a resource.
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.image = attachment.image;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = imageCreateInfo.format;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        result = vkCreateImageView(AppBase::GetDevice(), &imageViewCreateInfo, &attachment.imageView);
        if (result != VK_SUCCESS)
            FATAL("vkCreateImageView failed");
    }

    // Create the m_framebuffer.
    std::vector<VkImageView> attachments;
    for (auto& attachment : m_framebuffer.attachments)
        attachments.push_back(attachment.imageView);

    VkFramebufferCreateInfo framebufferCreateInfo = {};
    framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferCreateInfo.pAttachments = attachments.data();
    framebufferCreateInfo.width = width;
    framebufferCreateInfo.height = height;
    framebufferCreateInfo.layers = 1;
    auto result = vkCreateFramebuffer(AppBase::GetDevice(), &framebufferCreateInfo, &m_framebuffer.handle);
    if (result != VK_SUCCESS)
        FATAL("vkCreateFramebuffer failed");
}

void Subpasses::CreatePipeline()
{
    // Create the mrt pipeline.
    if (!AppBase::CreatePipeline({ { "../../Samples/Data/Shaders/Subpasses/Subpass0.vert", VK_SHADER_STAGE_VERTEX_BIT },
                                   { "../../Samples/Data/Shaders/Subpasses/Subpass0.frag", VK_SHADER_STAGE_FRAGMENT_BIT } },
                                   &m_subPass0Pipeline.pipeline, &m_subPass0Pipeline.shaderModules))
        FATAL("Create pipeline failed");

    // Create the grayscale pipeline.
    if (!AppBase::CreatePipeline({ { "../../Samples/Data/Shaders/Subpasses/Subpass1.vert", VK_SHADER_STAGE_VERTEX_BIT },
                                   { "../../Samples/Data/Shaders/Subpasses/Subpass1.frag", VK_SHADER_STAGE_FRAGMENT_BIT } },
                                   &m_subPass1Pipeline.pipeline, &m_subPass1Pipeline.shaderModules))
        FATAL("Create pipeline failed");
}

void Subpasses::CreateCommandBuffer()
{
    // Get the graphics queue handle.
    vkGetDeviceGraphicsQueue(AppBase::GetDevice(), 0, &m_graphicsQueue);

    // Create a command buffer handle.
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.queue = m_graphicsQueue;
    allocInfo.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(AppBase::GetDevice(), &allocInfo, &m_commandBuffer) != VK_SUCCESS)
        FATAL("vkAllocateCommandBuffers failed");

    // Begin command buffer recording.
    if (vkBeginCommandBuffer(m_commandBuffer, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT) != VK_SUCCESS)
        FATAL("vkBeginCommandBuffer failed");

    // Set the viewport state and dimensions.
    int width, height;
    AppBase::GetWindowSize(&width, &height);
    VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
    VkRect2D scissor = { { 0, 0 },{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) } };
    vkCmdSetViewport(m_commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(m_commandBuffer, 0, 1, &scissor);
    vkCmdSetViewportState(m_commandBuffer, 1);

    // Define clear values for the framebuffer attachments.
    std::array<VkAttachmentReference, 5> attachmentReferences = { {
        { VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, { 0.0f, 0.0f, 0.0f, 0.0f } },
        { VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, { 0.0f, 0.0f, 0.0f, 0.0f } },
        { VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, { 0.0f, 0.0f, 0.0f, 0.0f } },
        { VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, { 0.0f, 0.0f, 0.0f, 0.0f } },
        { VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, { 1.0f, 0 } }
    } };
   
    // Begin a new render pass.
    // First subpass renders the spinning quad and stores each color channel in a separate framebuffer attachment (attachments [1:3]).
    // See SubpassesMrt.frag.
    VkRenderPassBeginInfo beginInfo = {};
    beginInfo.framebuffer = m_framebuffer.handle;
    beginInfo.attachmentCount = static_cast<uint32_t>(attachmentReferences.size());
    beginInfo.pAttachments = attachmentReferences.data();
    vkCmdBeginRenderPass(m_commandBuffer, &beginInfo);
 
    // Bind the pipeline and associated resources.
    vkCmdBindPipeline(m_commandBuffer, m_subPass0Pipeline.pipeline);
    vkCmdBindBuffer(m_commandBuffer, m_uniformBuffer, 0, 0, 0);
    vkCmdBindImageView(m_commandBuffer, m_imageView, m_sampler, 0, 1, 0);

    // Set depth stencil state.
    VkPipelineDepthStencilState depthStencilState = {};
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    vkCmdSetDepthStencilState(m_commandBuffer, &depthStencilState);

    // Bind the vertex buffer and index buffers.
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, &m_vertexBuffer, &offset);
    vkCmdBindIndexBuffer(m_commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // Draw the quad.
    vkCmdDrawIndexed(m_commandBuffer, 6, 1, 0, 0, 0);

    // Begin the second subpass where the previous color attachment channels are combined into the final color output (attachment 0).
    // See SubapssesCombined.frag.
    vkCmdNextSubpass(m_commandBuffer);

    // Bind the pipeline.
    vkCmdBindPipeline(m_commandBuffer, m_subPass1Pipeline.pipeline);
    
    // Disable depth test and write.
    depthStencilState.depthTestEnable = VK_FALSE;
    depthStencilState.depthWriteEnable = VK_FALSE;
    vkCmdSetDepthStencilState(m_commandBuffer, &depthStencilState);

    // Set primitive topology to triangel strip.
    VkPipelineInputAssemblyState inputAssemblyState = {};
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    vkCmdSetInputAssemblyState(m_commandBuffer, &inputAssemblyState);

    // Draw a fullscreen quad (does not use bound vertex or index buffers).
    vkCmdDraw(m_commandBuffer, 4, 1, 0, 0);

    // End the render pass.
    vkCmdEndRenderPass(m_commandBuffer);

    // End command buffer recording.
    if (vkEndCommandBuffer(m_commandBuffer) != VK_SUCCESS)
        FATAL("vkEndCommandBuffer failed");
}