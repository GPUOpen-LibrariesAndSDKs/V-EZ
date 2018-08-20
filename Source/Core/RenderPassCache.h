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
#include <tuple>
#include <map>
#include <atomic>
#include "VEZ.h"
#include "Utility/SpinLock.h"
#include "StreamEncoder.h"

namespace vez
{
    class Device;

    typedef std::vector<uint64_t> RenderPassHash;

    class RenderPass
    {
    public:
        RenderPass(const RenderPassHash& hash, VkRenderPass handle, uint32_t colorAtachmentCount)
            : m_hash(hash)
            , m_handle(handle)
            , m_colorAttachmentCount(colorAtachmentCount)
        {}

        const RenderPassHash& GetHash() const { return m_hash; }

        VkRenderPass GetHandle() const { return m_handle; }

        uint32_t GetColorAttachmentCount() const { return m_colorAttachmentCount; }

    private:
        RenderPassHash m_hash = {};
        VkRenderPass m_handle = VK_NULL_HANDLE;
        uint32_t m_colorAttachmentCount = 0;
    };

    class RenderPassCache
    {
    public:
        RenderPassCache(Device* device);

        ~RenderPassCache();

        VkResult CreateRenderPass(const RenderPassDesc* pDesc, RenderPass** ppRenderPass);

        void DestroyRenderPass(RenderPass* renderPass);

        void DestroyUnusedRenderPasses();

    private:
        Device* m_device = nullptr;

        struct RenderPassAllocation
        {
            RenderPass* renderPass;
            uint32_t references;
        };

        std::map<RenderPassHash, RenderPassAllocation> m_renderPasses;
        SpinLock m_spinLock;
    };    
}