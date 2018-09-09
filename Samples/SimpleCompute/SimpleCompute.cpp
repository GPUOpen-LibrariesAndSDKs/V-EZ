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
#include <random>
#include <iostream>
#include <fstream>
#include <array>
#include <chrono>
#include "SimpleCompute.h"

#define FATAL(msg) { std::cout << msg << "\n"; AppBase::Exit(); return; }

struct UniformBuffer
{
    glm::mat4 view;
    glm::mat4 projection;
};

SimpleCompute::SimpleCompute()
    : AppBase("SimpleCompute Sample", 800, 600, 0, true)
{

}

void SimpleCompute::Initialize()
{
    CreateVertices();
    CreateUniformBuffer();
    CreatePipelines();
    CreateCommandBuffer();
}

void SimpleCompute::Cleanup()
{    
    auto device = AppBase::GetDevice();

    vezDestroyBuffer(device, m_vertexBuffer);
    vezDestroyBuffer(device, m_uniformBuffer);

    vezDestroyPipeline(device, m_computePipeline.pipeline);
    for (auto shaderModule : m_computePipeline.shaderModules)
        vezDestroyShaderModule(device, shaderModule);
    
    vezDestroyPipeline(device, m_graphicsPipeline.pipeline);
    for (auto shaderModule : m_graphicsPipeline.shaderModules)
        vezDestroyShaderModule(device, shaderModule);

    vezFreeCommandBuffers(device, 1, &m_commandBuffer);
}

void SimpleCompute::Draw()
{
    // Begin command buffer recording.
    vezBeginCommandBuffer(m_commandBuffer, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

    // Dispatch compute pipeline.
    vezCmdBindPipeline(m_computePipeline.pipeline);
    vezCmdBindBuffer(m_vertexBuffer, 0, VK_WHOLE_SIZE, 0, 0, 0);
    
    struct
    {
        int vertexCount;
        float theta;
    } pushConstants;

    pushConstants.vertexCount = static_cast<int>(m_vertexCount);
    pushConstants.theta = m_elapsedTime * 0.5f;
    vezCmdPushConstants(0, sizeof(pushConstants), reinterpret_cast<const void*>(&pushConstants));

    auto groupCountX = static_cast<uint32_t>(ceilf(m_vertexCount / 64.0f));
    vezCmdDispatch(groupCountX, 1, 1);

    // Get the window dimensions.
    int width, height;
    AppBase::GetWindowSize(&width, &height);

    // Update the ubo.
    float aspect = width / static_cast<float>(height);
    UniformBuffer ub = {};
    ub.view = glm::mat4(1.0f);
    ub.projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f);
    vezCmdUpdateBuffer(m_uniformBuffer, 0, sizeof(UniformBuffer), reinterpret_cast<const void*>(&ub));

    // Set the viewport and scissor states.
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

    // Set the primitive topology to points.
    VezInputAssemblyState inputState = {};
    inputState.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    vezCmdSetInputAssemblyState(&inputState);

    // Bind the pipeline and associated resources.
    vezCmdBindPipeline(m_graphicsPipeline.pipeline);
    vezCmdBindBuffer(m_uniformBuffer, 0, VK_WHOLE_SIZE, 0, 0, 0);

    // Bind the vertex buffer.
    VkDeviceSize offset = 0;
    vezCmdBindVertexBuffers(0, 1, &m_vertexBuffer, &offset);

    // Draw the vertices.
    vezCmdDraw(m_vertexCount, 1, 0, 0);

    // End the render pass.
    vezCmdEndRenderPass();

    // End command buffer recording.
    vezEndCommandBuffer();

    // Submit the command buffer to the graphics queue.
    VezSubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffer;

    // Request a wait semaphore to pass to present so it waits for rendering to complete.
    VkSemaphore semaphore = VK_NULL_HANDLE;
    //submitInfo.signalSemaphoreCount = 1;
    //submitInfo.pSignalSemaphores = &semaphore;
    if (vezQueueSubmit(m_graphicsQueue, 1, &submitInfo, nullptr) != VK_SUCCESS)
        FATAL("vkQueueSubmit failed");

    vezDeviceWaitIdle(AppBase::GetDevice());

    // Present the swapchain framebuffer to the window.
    VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    auto swapchain = AppBase::GetSwapchain();
    auto srcImage = AppBase::GetColorAttachment();

    VezPresentInfo presentInfo = {};
    //presentInfo.waitSemaphoreCount = 1;
    //presentInfo.pWaitSemaphores = &semaphore;
    //presentInfo.pWaitDstStageMask = &waitDstStageMask;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImages = &srcImage;
    if (vezQueuePresent(m_graphicsQueue, &presentInfo) != VK_SUCCESS)
        FATAL("vezQueuePresentKHR failed");

    // Wait for device to be idle so command buffer can be re-recorded.
    vkDeviceWaitIdle(AppBase::GetDevice());
}

void SimpleCompute::OnResize(int width, int height)
{
    // Re-create command buffer.
    vezFreeCommandBuffers(AppBase::GetDevice(), 1, &m_commandBuffer);
    CreateCommandBuffer();
}

void SimpleCompute::Update(float timeElapsed)
{
    m_elapsedTime = timeElapsed;
}

void SimpleCompute::CreateVertices()
{
    // Generate random vertex positions within [-1, 1] along xy axes.
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    std::vector<glm::vec4> vertices(m_vertexCount);
    for (auto i = 0U; i < m_vertexCount; ++i)
    {
        float radius = dist(mt);
        float theta = dist(mt) * 2.0f * 3.14159265f;
        vertices[i] = glm::vec4(cos(theta) * radius, sin(theta) * radius, 0.0f, 1.0f);
    }

    // Create the device side vertex buffer.
    VezBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.size = sizeof(glm::vec4) * vertices.size();
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    auto result = vezCreateBuffer(AppBase::GetDevice(), VEZ_MEMORY_GPU_ONLY, &bufferCreateInfo, &m_vertexBuffer);
    if (result != VK_SUCCESS)
        FATAL("vkCreateBuffer failed for vertex buffer");

    // Upload the host side data.
    result = vezBufferSubData(AppBase::GetDevice(), m_vertexBuffer, 0, sizeof(glm::vec4) * vertices.size(), reinterpret_cast<const void*>(vertices.data()));
    if (result != VK_SUCCESS)
        FATAL("vkBufferSubData failed for vertex buffer");
}

void SimpleCompute::CreateUniformBuffer()
{
    // Create a buffer for storing per frame matrices.
    VezBufferCreateInfo createInfo = {};
    createInfo.size = sizeof(UniformBuffer);
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (vezCreateBuffer(AppBase::GetDevice(), VEZ_MEMORY_CPU_TO_GPU, &createInfo, &m_uniformBuffer) != VK_SUCCESS)
        FATAL("vkCreateBuffer failed for uniform buffer");
}

void SimpleCompute::CreatePipelines()
{
    // Create the compute pipeline.
    if (!AppBase::CreatePipeline(
    { { "../../Samples/Data/Shaders/SimpleCompute/SimpleCompute.comp", VK_SHADER_STAGE_COMPUTE_BIT } },
        &m_computePipeline.pipeline, &m_computePipeline.shaderModules))
    {
        AppBase::Quit();
    }

    // Create the graphics pipeline.
    if (!AppBase::CreatePipeline(
        { { "../../Samples/Data/Shaders/SimpleCompute/SimpleCompute.vert", VK_SHADER_STAGE_VERTEX_BIT },
        { "../../Samples/Data/Shaders/SimpleCompute/SimpleCompute.frag", VK_SHADER_STAGE_FRAGMENT_BIT } },
        &m_graphicsPipeline.pipeline, &m_graphicsPipeline.shaderModules))
    {
        AppBase::Quit();
    }
}

void SimpleCompute::CreateCommandBuffer()
{
    // Get the graphics queue handle.
    vezGetDeviceGraphicsQueue(AppBase::GetDevice(), 0, &m_graphicsQueue);

    // Create a command buffer handle.
    VezCommandBufferAllocateInfo allocInfo = {};
    allocInfo.queue = m_graphicsQueue;
    allocInfo.commandBufferCount = 1;
    if (vezAllocateCommandBuffers(AppBase::GetDevice(), &allocInfo, &m_commandBuffer) != VK_SUCCESS)
        FATAL("vkAllocateCommandBuffers failed");
}