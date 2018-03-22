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
#include <random>
#include <iostream>
#include <fstream>
#include <array>
#include "OcclusionCulling.h"

#define FATAL(msg) { std::cout << msg << "\n"; AppBase::Exit(); return; }

struct Vertex
{
    float x, y, z;
    float nx, ny, nz;
};

struct CameraMatrices
{
    glm::mat4 view;
    glm::mat4 projection;
};

struct ModelData
{
    glm::mat4 transform;
    glm::vec4 color;
    glm::vec4 bboxMin;
    glm::vec4 bboxMax;
};

OcclusionCulling::OcclusionCulling()
    : AppBase("Occlusion Culling Sample", 500, 400, 0)
{

}

void OcclusionCulling::Initialize()
{
    CreatePipelines();
    CreateModel();
    CreateBuffers();
    CreateCommandBuffers();
    OcclusionTestPass();
    VisibilityReadbackPass();
}

void OcclusionCulling::Cleanup()
{    
    auto device = AppBase::GetDevice();
    
    if (m_fence != VK_NULL_HANDLE)
        vkDestroyFence(device, m_fence);

    vkFreeCommandBuffers(device, 1, &m_visibilityReadbackCmdBuf);
    vkFreeCommandBuffers(device, 1, &m_sceneRenderCmdBuf);
    vkFreeCommandBuffers(device, 1, &m_occlusionTestCmdBuf);    

    vkDestroyBuffer(device, m_occluderBuffer);
    vkDestroyBuffer(device, m_modelDataBuffer);
    vkDestroyBuffer(device, m_visibilityReadbackBuffer);
    vkDestroyBuffer(device, m_visibilityBuffer);
    vkDestroyBuffer(device, m_cameraMatricesBuffer);

    m_model.Destroy();

    vkDestroyPipeline(device, m_occluderPipeline.pipeline);
    for (auto shaderModule : m_occluderPipeline.shaderModules)

        vkDestroyShaderModule(device, shaderModule);
    vkDestroyPipeline(device, m_occlusionScenePipeline.pipeline);
    for (auto shaderModule : m_occlusionScenePipeline.shaderModules)
        vkDestroyShaderModule(device, shaderModule);

    vkDestroyPipeline(device, m_occlusionTestPipeline.pipeline);
    for (auto shaderModule : m_occlusionTestPipeline.shaderModules)
        vkDestroyShaderModule(device, shaderModule);
}

void OcclusionCulling::Draw()
{
    // Render scene every frame.
    SceneRenderPass();

    // Semaphore handles that need to be returned by each submit.
    std::array<VkSemaphore, 3> semaphores;

    // Submit the scene render command buffer to the graphics queue.
    VkSubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_sceneRenderCmdBuf;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphores[0];
    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, nullptr) != VK_SUCCESS)
        FATAL("vkQueueSubmit failed");

    // Submit the occlusion test command buffer to the graphics queue.
    submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_occlusionTestCmdBuf;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &semaphores[0];
    VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &stageFlags;
    submitInfo.signalSemaphoreCount = 2;
    submitInfo.pSignalSemaphores = &semaphores[1];
    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, nullptr) != VK_SUCCESS)
        FATAL("vkQueueSubmit failed");

    // Submit the visibility readback command buffer to the transfer queue.
    submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_visibilityReadbackCmdBuf;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &semaphores[1];
    submitInfo.pWaitDstStageMask = &stageFlags;    
    if (vkQueueSubmit(m_transferQueue, 1, &submitInfo, &m_fence) != VK_SUCCESS)
        FATAL("vkQueueSubmit failed");

    // Present the swapchain framebuffer to the window.
    VkPresentInfo presentInfo = { AppBase::GetColorAttachment(), 1, &semaphores[2], 0, nullptr };
    if (vkQueuePresent(m_graphicsQueue, &presentInfo) != VK_SUCCESS)
        FATAL("vkQueuePresentKHR failed");    

    // Destroy all of the wait semaphores.
    for (auto semaphore : semaphores)
        vkDestroySemaphore(AppBase::GetDevice(), semaphore);
}

void OcclusionCulling::OnMouseMove(int x, int y)
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

void OcclusionCulling::OnMouseScroll(float x, float y)
{
    float dz = (y > 0) ? 0.95f : 1.05f;
    m_cameraZoom *= dz;
}

void OcclusionCulling::OnResize(int width, int height)
{
    OcclusionTestPass();
}

void OcclusionCulling::Update(float timeElapsed)
{
    //vkDeviceWaitIdle(AppBase::GetDevice());

    // Get the current window dimensions.
    int width, height;
    AppBase::GetWindowSize(&width, &height);

    // Update the camera matrices.
    CameraMatrices cm = {};
   
    cm.view = glm::translate(glm::vec3(0.0f, 0.0f, -3.5f));
    cm.view = glm::rotate(cm.view, glm::radians(m_cameraRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    cm.view = glm::rotate(cm.view, glm::radians(m_cameraRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    cm.projection = glm::scale(glm::vec3(1.0f, -1.0f, 1.0f)) * glm::perspective(glm::radians(60.0f), width / static_cast<float>(height), 1.0f, 10.0f);

    // Map the camera matrices buffer for writing.
    void* data = nullptr;
    auto result = vkMapBuffer(AppBase::GetDevice(), m_cameraMatricesBuffer, 0, sizeof(CameraMatrices), &data);
    if (result != VK_SUCCESS)
        FATAL("vkMapBuffer failed");

    // Write the new matrices.
    memcpy(data, &cm, sizeof(CameraMatrices));

    // Flush the writes.
    VkMappedBufferRange bufferRange = {};
    bufferRange.buffer = m_cameraMatricesBuffer;
    bufferRange.offset = 0;
    bufferRange.size = sizeof(CameraMatrices);
    vkFlushMappedBufferRanges(AppBase::GetDevice(), 1, &bufferRange);

    // Unmap the buffer.
    vkUnmapBuffer(AppBase::GetDevice(), m_cameraMatricesBuffer);
}

void OcclusionCulling::CreatePipelines()
{
    // Create occlusion test pipeline.
    if (!AppBase::CreatePipeline(
        { { "../../Samples/Data/Shaders/OcclusionCulling/OcclusionTest.vert", VK_SHADER_STAGE_VERTEX_BIT },
        { "../../Samples/Data/Shaders/OcclusionCulling/OcclusionTest.geom", VK_SHADER_STAGE_GEOMETRY_BIT },
        { "../../Samples/Data/Shaders/OcclusionCulling/OcclusionTest.frag", VK_SHADER_STAGE_FRAGMENT_BIT } },
        &m_occlusionTestPipeline.pipeline, &m_occlusionTestPipeline.shaderModules))
    {
        FATAL("Errors compiling pipeline");
    }

    // Create occlusion scene pipeline.
    if (!AppBase::CreatePipeline(
    { { "../../Samples/Data/Shaders/OcclusionCulling/OcclusionScene.vert", VK_SHADER_STAGE_VERTEX_BIT },
    { "../../Samples/Data/Shaders/OcclusionCulling/OcclusionScene.frag", VK_SHADER_STAGE_FRAGMENT_BIT } },
        &m_occlusionScenePipeline.pipeline, &m_occlusionScenePipeline.shaderModules))
    {
        FATAL("Errors compiling pipeline");
    }

    // Create occluder pipeline.
    if (!AppBase::CreatePipeline(
    { { "../../Samples/Data/Shaders/OcclusionCulling/Occluder.vert", VK_SHADER_STAGE_VERTEX_BIT },
    { "../../Samples/Data/Shaders/OcclusionCulling/Occluder.frag", VK_SHADER_STAGE_FRAGMENT_BIT } },
        &m_occluderPipeline.pipeline, &m_occluderPipeline.shaderModules))
    {
        FATAL("Errors compiling pipeline");
    }
}

void OcclusionCulling::CreateModel()
{
    if (!m_model.LoadFromFile("../../Samples/Data/Models/buddha.obj", AppBase::GetDevice()))
        FATAL("Model load from file failed");
}

void OcclusionCulling::CreateBuffers()
{
    // Create a buffer for storing per frame matrices.
    VkBufferCreateInfo createInfo = {};
    createInfo.size = sizeof(CameraMatrices);
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (vkCreateBuffer(AppBase::GetDevice(), VK_MEMORY_CPU_TO_GPU, &createInfo, &m_cameraMatricesBuffer) != VK_SUCCESS)
        FATAL("vkCreateBuffer failed for camera matrices uniform buffer");

    // Create a buffer for storing per model visiblity;    
    createInfo.size = sizeof(uint32_t) * m_instanceCount;
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (vkCreateBuffer(AppBase::GetDevice(), VK_MEMORY_GPU_ONLY, &createInfo, &m_visibilityBuffer) != VK_SUCCESS)
        FATAL("vkCreateBuffer failed for visibility storage buffer");

    // Create host side buffer for visibility readback.
    createInfo.size = sizeof(uint32_t) * m_instanceCount;
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (vkCreateBuffer(AppBase::GetDevice(), VK_MEMORY_CPU_ONLY, &createInfo, &m_visibilityReadbackBuffer) != VK_SUCCESS)
        FATAL("vkCreateBuffer failed for visibility readback buffer");

    // Create a buffer for storing per model data.
    createInfo.size = sizeof(ModelData) * m_instanceCount;
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if (vkCreateBuffer(AppBase::GetDevice(), VK_MEMORY_GPU_ONLY, &createInfo, &m_modelDataBuffer) != VK_SUCCESS)
        FATAL("vkCreateBuffer failed for visibility storeage buffer");

    // Generate random data for each model instance.
    std::minstd_rand rand;
    rand.seed(static_cast<uint32_t>(time(nullptr)));

    std::vector<ModelData> modelData(m_instanceCount);
    for (auto i = 0U; i < modelData.size(); ++i)
    {
        float scale = ((rand() % 100) / 100.0f) * 0.1f + 0.05f;
        float yaw = static_cast<float>(rand() % 360);
        float pitch = static_cast<float>(rand() % 360);
        float x = (rand() % 100) / 25.0f - 2.0f;    // [-2, 2]
        float y = (rand() % 100) / 50.0f - 1.0f;    // [-1, 1]
        float z = (rand() % 100) / 25.0f - 3.5f;    // [-3.5, 0.5]
        float r = (rand() % 255) / 255.0f;
        float g = (rand() % 255) / 255.0f;
        float b = (rand() % 255) / 255.0f;

        modelData[i].transform = glm::translate(glm::vec3(x, y, z))
            * glm::rotate(glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f))
            * glm::rotate(glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f))
            * glm::scale(glm::vec3(scale, scale, scale))
            * glm::rotate(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        modelData[i].color = glm::vec4(r, g, b, 1.0f);
        modelData[i].bboxMin = glm::vec4(m_model.GetBoundingBox().min, 1.0f);
        modelData[i].bboxMax = glm::vec4(m_model.GetBoundingBox().max, 1.0f);
    }

    // Upload the data to the GPU side buffer.
    vkBufferSubData(AppBase::GetDevice(), m_modelDataBuffer, 0, sizeof(ModelData) * modelData.size(), modelData.data());

    // Create a buffer for the scene occluder.
    createInfo.size = sizeof(Vertex) * 6;
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (vkCreateBuffer(AppBase::GetDevice(), VK_MEMORY_GPU_ONLY, &createInfo, &m_occluderBuffer) != VK_SUCCESS)
        FATAL("vkCreateBuffer failed for visibility storeage buffer");

    // Upload the vertex data for the scene occluder.
    std::array<Vertex, 6> occluderVertices = { {
        { -1.5f, 1.0f, 0.5f, 0.0f, 0.0f, 1.0f },
        { 1.5f, 1.0f, 0.5f, 0.0f, 0.0f, 1.0f },
        { -1.5f, -1.0f, 0.5f, 0.0f, 0.0f, 1.0f },
        { -1.5f, -1.0f, 0.5f, 0.0f, 0.0f, 1.0f },
        { 1.5f, 1.0f, 0.5f, 0.0f, 0.0f, 1.0f },
        { 1.5f, -1.0f, 0.5f, 0.0f, 0.0f, 1.0f }
    } };
    vkBufferSubData(AppBase::GetDevice(), m_occluderBuffer, 0, sizeof(Vertex) * 6, occluderVertices.data());
}

void OcclusionCulling::CreateCommandBuffers()
{
    // Get the graphics and transfer queue handles.
    vkGetDeviceGraphicsQueue(AppBase::GetDevice(), 0, &m_graphicsQueue);
    vkGetDeviceTransferQueue(AppBase::GetDevice(), 0, &m_transferQueue);

    // Create a command buffer handle for the occlusion test pass.
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.queue = m_graphicsQueue;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(AppBase::GetDevice(), &allocInfo, &m_occlusionTestCmdBuf) != VK_SUCCESS)
        FATAL("vkAllocateCommandBuffers failed"); 

    // Create a command buffer handle for the scene render pass.
    if (vkAllocateCommandBuffers(AppBase::GetDevice(), &allocInfo, &m_sceneRenderCmdBuf) != VK_SUCCESS)
        FATAL("vkAllocateCommandBuffers failed");

    // Create a command buffer handle for the visibility data readback.
    allocInfo.queue = m_transferQueue;
    if (vkAllocateCommandBuffers(AppBase::GetDevice(), &allocInfo, &m_visibilityReadbackCmdBuf) != VK_SUCCESS)
        FATAL("vkAllocateCommandBuffers failed");

    // Pre-record commands for the occlusion test and visiblity readback passes.
    OcclusionTestPass();
    VisibilityReadbackPass();
}

void OcclusionCulling::SceneRenderPass()
{    
    // Get the visibility data from the last frame.
    uint32_t* visibilityData = nullptr;
    if (m_fence != VK_NULL_HANDLE)
    {
        if (vkWaitForFences(AppBase::GetDevice(), 1, &m_fence, VK_TRUE, ~0) == VK_SUCCESS)
        {
            vkMapBuffer(AppBase::GetDevice(), m_visibilityReadbackBuffer, 0, sizeof(uint32_t) * m_instanceCount, reinterpret_cast<void**>(&visibilityData));
            vkDestroyFence(AppBase::GetDevice(), m_fence);
            m_fence = VK_NULL_HANDLE;
        }
    }

    // Record commands.
    if (vkBeginCommandBuffer(m_sceneRenderCmdBuf, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT) != VK_SUCCESS)
        FATAL("vkBeginCommandBuffer failed");

    // Get the current window dimensions.
    int width, height;
    AppBase::GetWindowSize(&width, &height);

    // Set the viewport state and dimensions.    
    VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
    VkRect2D scissor = { { 0, 0 },{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) } };
    vkCmdSetViewport(m_sceneRenderCmdBuf, 0, 1, &viewport);
    vkCmdSetScissor(m_sceneRenderCmdBuf, 0, 1, &scissor);
    vkCmdSetViewportState(m_sceneRenderCmdBuf, 1);

    // Define clear values for the swapchain's color and depth attachments.
    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color.float32[0] = 0.3f;
    clearValues[0].color.float32[1] = 0.3f;
    clearValues[0].color.float32[2] = 0.3f;
    clearValues[1].depthStencil.depth = 1.0f;

    // Define clear values for the swapchain's color and depth attachments.
    std::array<VkAttachmentReference, 2> attachmentReferences = {};
    attachmentReferences[0].clearValue.color = { 0.3f, 0.3f, 0.3f, 0.0f };
    attachmentReferences[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentReferences[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentReferences[1].clearValue.depthStencil.depth = 1.0f;
    attachmentReferences[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentReferences[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // Begin a render pass.
    VkRenderPassBeginInfo beginInfo = {};
    beginInfo.framebuffer = AppBase::GetFramebuffer();
    beginInfo.attachmentCount = static_cast<uint32_t>(attachmentReferences.size());
    beginInfo.pAttachments = attachmentReferences.data();
    vkCmdBeginRenderPass(m_sceneRenderCmdBuf, &beginInfo);

    // Set rasterization state.
    VkPipelineRasterizationState rasterizationState = {};
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
    vkCmdSetRasterizationState(m_sceneRenderCmdBuf, &rasterizationState);

    // Set depth stencil state.
    VkPipelineDepthStencilState depthStencilState = {};
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    vkCmdSetDepthStencilState(m_sceneRenderCmdBuf, &depthStencilState);

    // Draw the occluder quad.
    vkCmdBindPipeline(m_sceneRenderCmdBuf, m_occluderPipeline.pipeline);
    vkCmdBindBuffer(m_sceneRenderCmdBuf, m_cameraMatricesBuffer, 0, 0, 0);
    VkDeviceSize zero = 0;
    vkCmdBindVertexBuffers(m_sceneRenderCmdBuf, 0, 1, &m_occluderBuffer, &zero);
    vkCmdDraw(m_sceneRenderCmdBuf, 6, 1, 0, 0);

    // Draw the model.
    vkCmdBindPipeline(m_sceneRenderCmdBuf, m_occlusionScenePipeline.pipeline);
    vkCmdBindBuffer(m_sceneRenderCmdBuf, m_cameraMatricesBuffer, 0, 0, 0);
    vkCmdBindBuffer(m_sceneRenderCmdBuf, m_modelDataBuffer, 0, 1, 0);
    uint32_t visibleObjectsCount = 0;
    for (auto i = 0U; i < m_instanceCount; ++i)
    {
        bool visible = (!visibilityData || visibilityData[i] != 0);
        if (visible)
        {
            vkCmdPushConstants(m_sceneRenderCmdBuf, 0, sizeof(uint32_t), &i);
            m_model.Draw(m_sceneRenderCmdBuf);
            ++visibleObjectsCount;
        }
    }

    // Display the number of visible objects in the window title bar.
    AppBase::SetWindowTitle("Visible objects " + std::to_string(visibleObjectsCount));

    // End the render pass.
    vkCmdEndRenderPass(m_sceneRenderCmdBuf);

    // End recording.
    if (vkEndCommandBuffer(m_sceneRenderCmdBuf) != VK_SUCCESS)
        FATAL("vkEndCommandBuffer failed");

    // Unmap the visibility buffer and release the associated fence.
    if (visibilityData)    
        vkUnmapBuffer(AppBase::GetDevice(), m_visibilityReadbackBuffer);
    
    if (m_fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(AppBase::GetDevice(), m_fence);
        m_fence = VK_NULL_HANDLE;
    }
}

void OcclusionCulling::OcclusionTestPass()
{
    // Record commands.
    if (vkBeginCommandBuffer(m_occlusionTestCmdBuf, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT) != VK_SUCCESS)
        FATAL("vkBeginCommandBuffer failed");

    // Clear visibility buffer.
    vkCmdFillBuffer(m_occlusionTestCmdBuf, m_visibilityBuffer, 0, sizeof(uint32_t) * m_instanceCount, 0U);

    // Get the current window dimensions.
    int width, height;
    AppBase::GetWindowSize(&width, &height);

    // Set the viewport state and dimensions.    
    VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
    VkRect2D scissor = { { 0, 0 },{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) } };
    vkCmdSetViewport(m_occlusionTestCmdBuf, 0, 1, &viewport);
    vkCmdSetScissor(m_occlusionTestCmdBuf, 0, 1, &scissor);
    vkCmdSetViewportState(m_occlusionTestCmdBuf, 1);

    // Define clear values for the swapchain's color and depth attachments.
    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color.float32[0] = 0.3f;
    clearValues[0].color.float32[1] = 0.3f;
    clearValues[0].color.float32[2] = 0.3f;
    clearValues[1].depthStencil.depth = 1.0f;

    // Define clear values for the swapchain's color and depth attachments.
    std::array<VkAttachmentReference, 2> attachmentReferences = {};
    attachmentReferences[0].clearValue.color = { 0.3f, 0.3f, 0.3f, 0.0f };
    attachmentReferences[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachmentReferences[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentReferences[1].clearValue.depthStencil.depth = 1.0f;
    attachmentReferences[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachmentReferences[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // Begin a render pass.
    VkRenderPassBeginInfo beginInfo = {};
    beginInfo.framebuffer = AppBase::GetFramebuffer();
    beginInfo.attachmentCount = static_cast<uint32_t>(attachmentReferences.size());
    beginInfo.pAttachments = attachmentReferences.data();
    vkCmdBeginRenderPass(m_occlusionTestCmdBuf, &beginInfo);

    // Bind pipeline and resources.
    vkCmdBindPipeline(m_occlusionTestCmdBuf, m_occlusionTestPipeline.pipeline);
    vkCmdBindBuffer(m_occlusionTestCmdBuf, m_cameraMatricesBuffer, 0, 0, 0);
    vkCmdBindBuffer(m_occlusionTestCmdBuf, m_modelDataBuffer, 0, 1, 0);
    vkCmdBindBuffer(m_occlusionTestCmdBuf, m_visibilityBuffer, 0, 2, 0);

    // Set input assembly state.
    VkPipelineInputAssemblyState inputAssemblyState = {};
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    vkCmdSetInputAssemblyState(m_occlusionTestCmdBuf, &inputAssemblyState);

    // Set rasterization state.
    VkPipelineRasterizationState rasterizationState = {};
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.cullMode = VK_CULL_MODE_NONE;
    rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
    vkCmdSetRasterizationState(m_occlusionTestCmdBuf, &rasterizationState);

    // Set depth stencil state.
    VkPipelineDepthStencilState depthStencilState = {};
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_FALSE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    vkCmdSetDepthStencilState(m_occlusionTestCmdBuf, &depthStencilState);

    // Set color blend state.
    VkPipelineColorBlendAttachmentState blendAttachmentState = {};    
    VkPipelineColorBlendState colorBlendState = {};
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &blendAttachmentState;
    vkCmdSetColorBlendState(m_occlusionTestCmdBuf, &colorBlendState);

    // Draw the model.
    vkCmdDraw(m_occlusionTestCmdBuf, m_instanceCount, 1, 0, 0);

    // End the render pass.
    vkCmdEndRenderPass(m_occlusionTestCmdBuf);

    // End recording.
    if (vkEndCommandBuffer(m_occlusionTestCmdBuf) != VK_SUCCESS)
        FATAL("vkEndCommandBuffer failed");
}

void OcclusionCulling::VisibilityReadbackPass()
{
    // Record commands.
    if (vkBeginCommandBuffer(m_visibilityReadbackCmdBuf, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT) != VK_SUCCESS)
        FATAL("vkBeginCommandBuffer failed");

    // Copy device side visibility buffer to CPU side one.
    VkBufferCopy region = {};
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = sizeof(uint32_t) * m_instanceCount;
    vkCmdCopyBuffer(m_visibilityReadbackCmdBuf, m_visibilityBuffer, m_visibilityReadbackBuffer, 1, &region);

    // End recording.
    if (vkEndCommandBuffer(m_visibilityReadbackCmdBuf))
        FATAL("vkEndCommandBuffer failed");
}