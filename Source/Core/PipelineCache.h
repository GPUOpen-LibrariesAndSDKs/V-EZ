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

#include <vector>
#include <map>
#include <unordered_map>
#include "VEZ.h"
#include "Utility/SpinLock.h"
#include "GraphicsState.h"

namespace vez
{
    class Device;
    class Pipeline;
    class RenderPass;

    class PipelineCache
    {
    public:
        PipelineCache(Device* device);

        ~PipelineCache();

        VkResult GetHandle(const Pipeline* pipeline, const RenderPass* pRenderPass, const GraphicsState* pState, VkPipeline* pHandle);

    private:
        typedef std::vector<uint64_t> PipelinePermutationHash;

        VkResult CreateGraphicsPipeline(const Pipeline* pipeline, const RenderPass* pRenderPass, const GraphicsState* pState, VkPipeline* pHandle);

        VkResult CreateComputePipeline(const Pipeline* pipeline, VkPipeline* pHandle);

        PipelinePermutationHash GetPipelinePermutationHash(const Pipeline* pipeline, const RenderPass* pRenderPass, const GraphicsState* pState);

        Device* m_device = nullptr;
        std::map<PipelinePermutationHash, VkPipeline> m_allPipelinesCache;
        VkPipelineCache m_vulkanPipelineCache = VK_NULL_HANDLE;
        SpinLock m_spinLock;
    };    
}