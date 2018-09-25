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
#include "ShadowMapping.h"

#define FATAL(msg) { std::cout << msg << "\n"; AppBase::Exit(); return; }

struct CameraMatrices
{
    glm::mat4 viewProj;
    glm::mat4 viewProjInv;
};

struct LightMatrices
{
    glm::mat4 viewProj;
    glm::mat4 viewProjInv;
};

ShadowMapping::ShadowMapping()
    : AppBase("ShadowMapping Sample", 500, 400, 0, true)
{

}

void ShadowMapping::Initialize()
{
    CreatePipelines();
    CreateShadowMap();
    CreateSampler();
    CreateUniformBuffers();
    CreateModel();
    CreateCommandBuffer();
}

void ShadowMapping::Cleanup()
{    
    auto device = AppBase::GetDevice();

    vezDestroyPipeline(device, m_depthPassPipeline.pipeline);
    for (auto shaderModule : m_depthPassPipeline.shaderModules)
        vezDestroyShaderModule(device, shaderModule);

    vezDestroyPipeline(device, m_scenePassPipeline.pipeline);
    for (auto shaderModule : m_scenePassPipeline.shaderModules)
        vezDestroyShaderModule(device, shaderModule);

    vezDestroyFramebuffer(device, m_depthFramebuffer);
    vezDestroyImageView(device, m_depthImageView);
    vezDestroyImage(device, m_depthImage);
    vezDestroySampler(device, m_sampler);
    vezDestroyBuffer(device, m_cameraMatrices);
    vezDestroyBuffer(device, m_lightMatrices);
    m_model.Destroy();
    vezFreeCommandBuffers(device, 1, &m_commandBuffer);
}

void ShadowMapping::Draw()
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
    presentInfo.pImages = &srcImage;
    if (vezQueuePresent(m_graphicsQueue, &presentInfo) != VK_SUCCESS)
        FATAL("vezQueuePresentKHR failed");
}

void ShadowMapping::OnMouseMove(int x, int y)
{
    static struct
    {
        double x, y;
        int leftButton = 0;
    } mouseState;

    int leftButton = glfwGetMouseButton(AppBase::GetWindow(), GLFW_MOUSE_BUTTON_LEFT);
    if (leftButton == GLFW_PRESS)
    {
        if (!mouseState.leftButton)
        {
            mouseState.x = x;
            mouseState.y = y;
        }

        float dx = static_cast<float>(x - mouseState.x) * 0.4f;
        float dy = static_cast<float>(y - mouseState.y) * 0.4f;
        m_cameraRotation += glm::vec3(dy, dx, 0.0f);
        mouseState.x = x;
        mouseState.y = y;
    }

    mouseState.leftButton = leftButton;
    mouseState.x = x;
    mouseState.y = y;
}

void ShadowMapping::OnMouseScroll(float x, float y)
{
    float dz = (y > 0) ? 0.95f : 1.05f;
    m_cameraZoom *= dz;
}

void ShadowMapping::OnResize(int width, int height)
{
    // Re-create command buffer.
    vezFreeCommandBuffers(AppBase::GetDevice(), 1, &m_commandBuffer);
    CreateCommandBuffer();
}

void ShadowMapping::Update(float timeElapsed)
{
    // Get the current window dimensions.
    int width, height;
    AppBase::GetWindowSize(&width, &height);

    // Update the camera matrices.
    CameraMatrices cm = {};

    const auto& bbox = m_model.GetBoundingBox();
    auto bboxDim = bbox.max - bbox.min;
    auto bboxCenter = (bbox.max + bbox.min) * 0.5f;
    float bboxMaxDim = std::max(bboxDim.x, std::max(bboxDim.y, bboxDim.z));

    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -bboxDim.length() * 3.0f * m_cameraZoom));
    view = glm::rotate(view, glm::radians(m_cameraRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    view = glm::rotate(view, glm::radians(m_cameraRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 proj = glm::perspective(glm::radians(60.0f), width / static_cast<float>(height), 0.1f, bboxMaxDim * 5.0f);
    proj[1][1] *= -1.0f;

    cm.viewProj = proj * view;
    cm.viewProjInv = glm::inverse(cm.viewProj);

    // Update the memory via map and unmap since it was created as HOST_VISIBLE (i.e. VK_MEMORY_CPU_TO_GPU).
    void* data = nullptr;
    auto result = vezMapBuffer(AppBase::GetDevice(), m_cameraMatrices, 0, sizeof(CameraMatrices), &data);
    if (result != VK_SUCCESS)
        FATAL("vkMapBuffer failed");

    memcpy(data, &cm, sizeof(CameraMatrices));
    vezUnmapBuffer(AppBase::GetDevice(), m_cameraMatrices);

    // Update the light's matrices.
    static float lightRot = 0.0f;
    lightRot += timeElapsed * glm::radians(45.0f);
    
    LightMatrices lm = {};
    lm.viewProj = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, 1.0f)) *  glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, bboxMaxDim * 5.0f)
        * glm::lookAt(bboxCenter + glm::vec3(0.0f, bboxMaxDim * 2.0f, bboxMaxDim), bboxCenter, glm::vec3(0.0f, 1.0f, 0.0f))
        * glm::rotate(glm::mat4(1.0f), lightRot, glm::vec3(0.0f, 1.0, 0.0f));
    lm.viewProjInv = glm::inverse(lm.viewProj);
    result = vezMapBuffer(AppBase::GetDevice(), m_lightMatrices, 0, sizeof(LightMatrices), &data);
    if (result != VK_SUCCESS)
        FATAL("vkMapBuffer failed");

    memcpy(data, &lm, sizeof(LightMatrices));
    vezUnmapBuffer(AppBase::GetDevice(), m_lightMatrices);
}

void ShadowMapping::CreatePipelines()
{
    // Create depth pass pipeline.
    if (!AppBase::CreatePipeline(
        { {"../../Samples/Data/Shaders/ShadowMapping/DepthPass.vert", VK_SHADER_STAGE_VERTEX_BIT } },
        &m_depthPassPipeline.pipeline, &m_depthPassPipeline.shaderModules))
    {
        AppBase::Quit();
    }

    // Create scene pass pipeline.
    if (!AppBase::CreatePipeline(
        { { "../../Samples/Data/Shaders/ShadowMapping/ScenePass.vert", VK_SHADER_STAGE_VERTEX_BIT },
        { "../../Samples/Data/Shaders/ShadowMapping/ScenePass.frag", VK_SHADER_STAGE_FRAGMENT_BIT } },
        &m_scenePassPipeline.pipeline, &m_scenePassPipeline.shaderModules))
    {
        AppBase::Quit();
    }
}

void ShadowMapping::CreateShadowMap()
{
    // Create the device side image.
    VezImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_D32_SFLOAT;
    imageCreateInfo.extent = { 2048, 2048, 1 };
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    auto result = vezCreateImage(AppBase::GetDevice(), VEZ_MEMORY_GPU_ONLY, &imageCreateInfo, &m_depthImage);
    if (result != VK_SUCCESS)
        FATAL("vkCreateImage failed");

    // Create the image view.
    VezImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.image = m_depthImage;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = imageCreateInfo.format;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    result = vezCreateImageView(AppBase::GetDevice(), &imageViewCreateInfo, &m_depthImageView);
    if (result != VK_SUCCESS)
        FATAL("vkCreateImageView failed");

    // Create the framebuffer.
    VezFramebufferCreateInfo framebufferCreateInfo = {};
    framebufferCreateInfo.attachmentCount = 1;
    framebufferCreateInfo.pAttachments = &m_depthImageView;
    framebufferCreateInfo.width = 2048;
    framebufferCreateInfo.height = 2048;
    framebufferCreateInfo.layers = 1;
    result = vezCreateFramebuffer(AppBase::GetDevice(), &framebufferCreateInfo, &m_depthFramebuffer);
    if (result != VK_SUCCESS)
        FATAL("vkCreateFramebuffer failed");
}

void ShadowMapping::CreateSampler()
{
    VezSamplerCreateInfo createInfo = {};
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;  
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.maxAnisotropy = 1.0f;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = 1.0f;
    createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    auto result = vezCreateSampler(AppBase::GetDevice(), &createInfo, &m_sampler);
    if (result != VK_SUCCESS)
        FATAL("vkCreateSampler failed");
}

void ShadowMapping::CreateUniformBuffers()
{
    // Create a buffer for storing per frame matrices.
    VezBufferCreateInfo createInfo = {};
    createInfo.size = sizeof(CameraMatrices);
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (vezCreateBuffer(AppBase::GetDevice(), VEZ_MEMORY_CPU_TO_GPU, &createInfo, &m_cameraMatrices) != VK_SUCCESS)
        FATAL("vkCreateBuffer failed for camera matrices uniform buffer");

    // Create a buffer for storing the light's matrices.
    createInfo.size = sizeof(LightMatrices);
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (vezCreateBuffer(AppBase::GetDevice(), VEZ_MEMORY_CPU_TO_GPU, &createInfo, &m_lightMatrices) != VK_SUCCESS)
        FATAL("vkCreateBuffer failed for light matrices uniform buffer");
}

void ShadowMapping::CreateModel()
{
    if (!m_model.LoadFromFile("../../Samples/Data/Models/vulkanscene_shadow.dae", AppBase::GetDevice()))
        FATAL("Model load from file failed");
}

void ShadowMapping::CreateCommandBuffer()
{
    // Get the graphics queue handle.
    vezGetDeviceGraphicsQueue(AppBase::GetDevice(), 0, &m_graphicsQueue);

    // Create a command buffer handle.
    VezCommandBufferAllocateInfo allocInfo = {};
    allocInfo.queue = m_graphicsQueue;
    allocInfo.commandBufferCount = 1;
    if (vezAllocateCommandBuffers(AppBase::GetDevice(), &allocInfo, &m_commandBuffer) != VK_SUCCESS)
        FATAL("vkAllocateCommandBuffers failed");

    // Record commands.
    if (vezBeginCommandBuffer(m_commandBuffer, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT) != VK_SUCCESS)
        FATAL("vkBeginCommandBuffer failed");

    // Generate shadow map.
    DepthPass();

    // Render scene.
    ScenePass();

    // End recording.
    if (vezEndCommandBuffer() != VK_SUCCESS)
        FATAL("vkEndCommandBuffer failed");
}

void ShadowMapping::DepthPass()
{
    // Set the viewport state and dimensions.
    VkViewport viewport = { 0.0f, 0.0f, 2048.0f, 2048.0f, 0.0f, 1.0f };
    VkRect2D scissor = { { 0, 0 },{ 2048, 2048 } };
    vezCmdSetViewport(0, 1, &viewport);
    vezCmdSetScissor(0, 1, &scissor);
    vezCmdSetViewportState(1);

    // Set depth bias for geometry rendered into shadow map.
    vezCmdSetDepthBias(1.25f, 0.0f, 1.75f);

    // Define clear values for the swapchain's color and depth attachments.
    std::array<VezAttachmentReference, 1> attachmentReferences = {};
    attachmentReferences[0].clearValue.depthStencil.depth = 1.0f;
    attachmentReferences[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentReferences[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // Begin a render pass.
    VezRenderPassBeginInfo beginInfo = {};
    beginInfo.framebuffer = m_depthFramebuffer;
    beginInfo.attachmentCount = static_cast<uint32_t>(attachmentReferences.size());
    beginInfo.pAttachments = attachmentReferences.data();
    vezCmdBeginRenderPass(&beginInfo);

    // Bind the pipeline.
    vezCmdBindPipeline(m_depthPassPipeline.pipeline);

    // Bind resources.
    vezCmdBindBuffer(m_cameraMatrices, 0, VK_WHOLE_SIZE, 0, 0, 0);
    vezCmdBindBuffer(m_lightMatrices, 0, VK_WHOLE_SIZE, 0, 1, 0);

    // Set rasterization state.
    VezRasterizationState rasterizationState = {};
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationState.depthBiasEnable = VK_TRUE;
    vezCmdSetRasterizationState(&rasterizationState);

    // Set depth stencil state.
    VezDepthStencilState depthStencilState = {};
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    vezCmdSetDepthStencilState(&depthStencilState);

    // Draw the model.
    m_model.Draw();

    // End the render pass.
    vezCmdEndRenderPass();
}

void ShadowMapping::ScenePass()
{
    // Get the current window dimensions.
    int width, height;
    AppBase::GetWindowSize(&width, &height);

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
    beginInfo.framebuffer = AppBase::GetFramebuffer();
    beginInfo.attachmentCount = static_cast<uint32_t>(attachmentReferences.size());
    beginInfo.pAttachments = attachmentReferences.data();
    vezCmdBeginRenderPass(&beginInfo);

    // Bind the pipeline.
    vezCmdBindPipeline(m_scenePassPipeline.pipeline);

    // Bind resources.
    vezCmdBindImageView(m_depthImageView, m_sampler, 0, 2, 0);

    // Set rasterization state.
    VezRasterizationState rasterizationState = {};
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
    vezCmdSetRasterizationState(&rasterizationState);

    // Set depth stencil state.
    VezPipelineDepthStencilState depthStencilState = {};
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    vezCmdSetDepthStencilState(&depthStencilState);

    // Draw the model.
    m_model.Draw();

    // End the render pass.
    vezCmdEndRenderPass();
}