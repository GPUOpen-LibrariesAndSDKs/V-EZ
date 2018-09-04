
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

#include <stdint.h>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <VEZ.h>

class Model
{
public:
    struct BBox
    {
        glm::vec3 min = glm::vec3(FLT_MAX);
        glm::vec3 max = glm::vec3(-FLT_MAX);
    };

    ~Model();

    void Destroy();

    const BBox& GetBoundingBox() const { return m_bbox; }

    uint32_t GetIndexCount() { return m_indexCount; }
    
    uint32_t GetVertexCount() { return m_vertexCount; }

    bool LoadFromFile(const std::string& filename, VkDevice device);    

    void Draw(uint32_t instanceCount = 1);

private:
    struct Part
    {
        uint32_t vertexBase = 0;
        uint32_t vertexCount = 0;
        uint32_t indexBase = 0;
        uint32_t indexCount = 0;
    };

    VkDevice m_device = VK_NULL_HANDLE;
    std::vector<Part> m_parts;
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    uint32_t m_vertexCount = 0;
    uint32_t m_indexCount = 0;
    VezVertexInputFormat m_vertexInputFormat = VK_NULL_HANDLE;
    BBox m_bbox;
};