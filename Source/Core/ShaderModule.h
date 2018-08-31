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

#include <string>
#include <vector>
#include "VEZ.h"

namespace vez
{
    class Device;

    class ShaderModule
    {
    public:
        static VkResult Create(Device* pDevice, const VezShaderModuleCreateInfo* pCreateInfo, ShaderModule** ppShaderModule);

        ~ShaderModule();

        VkShaderModule GetHandle() const { return m_handle; }

        VkShaderStageFlagBits GetStage() const { return m_stage; }

        const std::string& GetEntryPoint() const { return m_entryPoint; }

        const std::vector<VezPipelineResource>& GetResources() const { return m_resources; }

        const std::string& GetInfoLog() const { return m_infoLog; }

        VkResult GetBinary(uint32_t* pLength, uint32_t* pBinary);

    private:
        Device* m_device = nullptr;
        VkShaderModule m_handle = VK_NULL_HANDLE;
        VkShaderStageFlagBits m_stage;
        std::string m_entryPoint;
        std::vector<uint32_t> m_spirv;
        std::vector<VezPipelineResource> m_resources;
        std::string m_infoLog;
    };    
}