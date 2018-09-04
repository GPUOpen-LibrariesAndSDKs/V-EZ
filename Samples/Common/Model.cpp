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
#include <array>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Model.h"

Model::~Model()
{
}

void Model::Destroy()
{
    if (m_vertexBuffer)
    {
        vezDestroyBuffer(m_device, m_vertexBuffer);
        m_vertexBuffer = VK_NULL_HANDLE;
    }

    if (m_indexBuffer)
    {
        vezDestroyBuffer(m_device, m_indexBuffer);
        m_indexBuffer = VK_NULL_HANDLE;
    }
}

bool Model::LoadFromFile(const std::string& filename, VkDevice device)
{
    // Save VulkanEZ device handle.
    m_device = device;

    // Load the model from disk.
    Assimp::Importer importer;
    const auto* scene = importer.ReadFile(filename.c_str(), aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_GenSmoothNormals);
    if (!scene)
        return false;
    
    // Each Assimp Mesh becomes a Part.  Pre-allocate space for all parts.
    m_parts.clear();
    m_parts.resize(scene->mNumMeshes);

    // Iterate over all meshes and fill in part info.
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    m_vertexCount = 0;
    m_indexCount = 0;
    for (auto i = 0U; i < scene->mNumMeshes; ++i)
    {                
        // Process the mesh.
        const auto* mesh = scene->mMeshes[i];
        
        // All parts share the same vertex and index buffer, so keep track of offsets.
        m_parts[i].vertexBase = m_vertexCount;
        m_parts[i].indexBase = m_vertexCount;// static_cast<uint32_t>(indices.size());

#if 1
        // Get base index based on vertices already stored in buffer.
        auto indexBase = m_vertexCount;

        // Iterate over the mesh's vertices.
        for (auto j = 0U; j < mesh->mNumVertices; ++j)
        {
            // Get the vertex attributes.
            const auto& pos = mesh->mVertices[j];
            const auto& normal = mesh->mNormals[j];

            // Fill the temporary vertex data vector.
            vertices.push_back(pos.x);
            vertices.push_back(pos.y);
            vertices.push_back(pos.z);
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);

            // Apply vertex to bounding box.
            m_bbox.min = glm::min(m_bbox.min, glm::vec3(pos.x, pos.y, pos.z));
            m_bbox.max = glm::max(m_bbox.max, glm::vec3(pos.x, pos.y, pos.z));
            
            // Increment vertex count.
            ++m_vertexCount;
        }

        // Iterate over mesh's indices.
        // static_cast<uint32_t>(indices.size());
        for (auto j = 0U; j < mesh->mNumFaces; ++j)
        {
            const auto& face = mesh->mFaces[j];
            if (face.mNumIndices != 3)
                continue;
            
            indices.push_back(indexBase + face.mIndices[0]);
            indices.push_back(indexBase + face.mIndices[1]);
            indices.push_back(indexBase + face.mIndices[2]);
            m_parts[i].indexCount += 3;
            m_indexCount += 3;
        }
#else
        for (auto j = 0U; j < mesh->mNumFaces; ++j)
        {
            const auto& face = mesh->mFaces[j];
            if (face.mNumIndices != 3)
                continue;

            for (auto k = 0; k < 3; ++k)
            {
                auto index = face.mIndices[k];
                const auto& pos = mesh->mVertices[index];
                const auto& normal = mesh->mNormals[index];

                vertices.push_back(pos.x);
                vertices.push_back(pos.y);
                vertices.push_back(pos.z);
                vertices.push_back(normal.x);
                vertices.push_back(normal.y);
                vertices.push_back(normal.z);

                m_bbox.min = glm::min(m_bbox.min, glm::vec3(pos.x, pos.y, pos.z));
                m_bbox.max = glm::max(m_bbox.max, glm::vec3(pos.x, pos.y, pos.z));

                ++m_vertexCount;
            }
        }
#endif
        // Increment total vertex count so far.
        //m_vertexCount += mesh->mNumVertices;
    }

    // Allocate a buffer for the vertices and upload the host data.
    VezBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferCreateInfo.size = sizeof(float) * vertices.size();
    auto result = vezCreateBuffer(device, VEZ_MEMORY_GPU_ONLY, &bufferCreateInfo, &m_vertexBuffer);
    if (result != VK_SUCCESS)
        return false;

    result = vezBufferSubData(device, m_vertexBuffer, 0, bufferCreateInfo.size, static_cast<const void*>(vertices.data()));
    if (result != VK_SUCCESS)
        return false;
#if 1
    // Allocate a buffer for the indices and upload the host data.
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    bufferCreateInfo.size = sizeof(float) * indices.size();
    result = vezCreateBuffer(device, VEZ_MEMORY_GPU_ONLY, &bufferCreateInfo, &m_indexBuffer);
    if (result != VK_SUCCESS)
        return false;

    result = vezBufferSubData(device, m_indexBuffer, 0, bufferCreateInfo.size, static_cast<const void*>(indices.data()));
    if (result != VK_SUCCESS)
        return false;
#endif
    // Create VulkanEZ vertex input format.
    VkVertexInputBindingDescription bindingDesc = { 0, sizeof(float) * 6, VK_VERTEX_INPUT_RATE_VERTEX };
    std::array<VkVertexInputAttributeDescription, 2> attribDesc = {
        VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
        VkVertexInputAttributeDescription{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3 }
    };
    VezVertexInputFormatCreateInfo vertexInputFormatCreateInfo = {};
    vertexInputFormatCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputFormatCreateInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputFormatCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDesc.size());
    vertexInputFormatCreateInfo.pVertexAttributeDescriptions = attribDesc.data();
    result = vezCreateVertexInputFormat(device, &vertexInputFormatCreateInfo, &m_vertexInputFormat);
    if (result != VK_SUCCESS)
        return false;

    // Return success.
    return true;
}

void Model::Draw(uint32_t instanceCount)
{
    // Bind the vertex and index buffers.
    VkDeviceSize offset = 0;
    vezCmdBindVertexBuffers(0, 1, &m_vertexBuffer, &offset);
    vezCmdBindIndexBuffer(m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // Set the vertex input format.
    vezCmdSetVertexInputFormat(m_vertexInputFormat);

#if 0
    // Draw each part.
    for (auto& part : m_parts)
        vkCmdDrawIndexed(commandBuffer, part.indexCount, instanceCount, part.indexBase, 0, 0);
#else
    // Draw the entire model in one call.
    vezCmdDrawIndexed(m_indexCount, 1, 0, 0, 0);
#endif
}