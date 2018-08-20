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
#include <stdio.h>
#include <cstring>
#include "Compiler/GLSLCompiler.h"
#include "Compiler/SPIRVReflection.h"
#include "Device.h"
#include "ShaderModule.h"

namespace vez
{
    VkResult ShaderModule::Create(Device* pDevice, const VezShaderModuleCreateInfo* pCreateInfo, ShaderModule** ppShaderModule)
    {
        // Create a new instance of the ShaderModule class.
        auto shaderModule = new ShaderModule;
        shaderModule->m_device = pDevice;
        shaderModule->m_stage = pCreateInfo->stage;

        // If application is passing in GLSL source code, compile to SPIR-V.
        VkResult result = VK_SUCCESS;
        std::vector<uint32_t> spirv;
        if (pCreateInfo->pGLSLSource)
        {
            // Compiling from GLSL source requires the entry point.
            if (!pCreateInfo->pEntryPoint)
            {
                delete shaderModule;
                return VK_ERROR_INITIALIZATION_FAILED;
            }

            // Compile the GLSL source.
            if (!CompileGLSL2SPIRV(pCreateInfo->stage, pCreateInfo->pGLSLSource, pCreateInfo->pEntryPoint, spirv, shaderModule->m_infoLog))
            {
                // Store ShaderModule object address so shader log can be retrived.  Set the native Vulkan object handle to the same memory address.
                shaderModule->m_handle = reinterpret_cast<VkShaderModule>(shaderModule);
                *ppShaderModule = shaderModule;
                return VK_ERROR_INITIALIZATION_FAILED;
            }

            // Save entry point name.
            shaderModule->m_entryPoint = pCreateInfo->pEntryPoint;
        }
        // Copy the spirv data into a separate vector.
        else
        {
            spirv.resize(pCreateInfo->codeSize / sizeof(uint32_t));
            memcpy(spirv.data(), pCreateInfo->pCode, pCreateInfo->codeSize);
        }

        // If GLSL compilation was successfull, or it wasn't needed, move onto the SPIR-V.
        if (result == VK_SUCCESS)
        {
            // Reflection all shader resouces.
            if (!SPIRVReflectResources(spirv, shaderModule->m_stage, shaderModule->m_resources))
                return VK_ERROR_INITIALIZATION_FAILED;

            // Create the Vulkan handle.
            VkShaderModuleCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.pNext = pCreateInfo->pNext;
            createInfo.codeSize = spirv.size() * 4;
            createInfo.pCode = spirv.data();
            result = vkCreateShaderModule(pDevice->GetHandle(), &createInfo, nullptr, &shaderModule->m_handle);
            if (result == VK_SUCCESS)
                *ppShaderModule = shaderModule;
        }

        // Return final result.
        return result;
    }

    ShaderModule::~ShaderModule()
    {
        // Handle case where GLSL compilation failed but ShaderModule class object was still created in order to retrieve info log.
        if (m_handle && m_handle != reinterpret_cast<VkShaderModule>(this))
            vkDestroyShaderModule(m_device->GetHandle(), m_handle, nullptr);
    }    
}