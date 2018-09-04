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
#include <iomanip>
#include <fstream>
#include <array>
#include <chrono>
#include "PipelineReflection.h"

#define FATAL(msg) { std::cout << msg << "\n"; AppBase::Exit(); return; }

PipelineReflection::PipelineReflection()
    : AppBase("PipelineReflection Sample", 128, 128, 0, false)
{
    
}

void PipelineReflection::Initialize()
{
    // Load the test compute shader.
    VezPipeline pipeline;
    std::vector<VkShaderModule> shaderModules;
    if (!AppBase::CreatePipeline(
        { { "../../Samples/Data/Shaders/PipelineReflection/Reflection.comp", VK_SHADER_STAGE_COMPUTE_BIT } },
        &pipeline, &shaderModules))
    {
        AppBase::Exit();
    }

    // Display full spirv reflection info.
    auto resourceCount = 0U;
    vezEnumeratePipelineResources(pipeline, &resourceCount, nullptr);

    std::vector<VezPipelineResource> resources(resourceCount);
    vezEnumeratePipelineResources(pipeline, &resourceCount, resources.data());

    for (auto& resource : resources)
    {
        std::cout << "name=" << resource.name << ", set=" << resource.set << ", binding=" << resource.binding << "\n";
        if (resource.pMembers)
            DisplayMember(1, resource.pMembers);
    }

    // Exit application.
    AppBase::Quit();
}

void PipelineReflection::DisplayMember(uint32_t level, const VezMemberInfo* pMemberInfo)
{
    std::cout << std::setw(level * 4) << "" << "name=" << pMemberInfo->name << ", offset=" << pMemberInfo->offset << ", size=" << pMemberInfo->size << ", vecsize=" << pMemberInfo->vecSize << ", columns=" << pMemberInfo->columns << "\n";

    if (pMemberInfo->pMembers)
        DisplayMember(level + 1, pMemberInfo->pMembers);

    if (pMemberInfo->pNext)
        DisplayMember(level, pMemberInfo->pNext);
}