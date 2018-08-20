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
#include <list>
#include <unordered_map>
#include "VEZ.h"

namespace vez
{
    struct PipelineBarrier
    {
        uint64_t streamPosition;
        VkPipelineStageFlags srcStageMask;
        VkPipelineStageFlags dstStageMask;
        std::vector<VkBufferMemoryBarrier> bufferBarriers;
        std::vector<VkImageMemoryBarrier> imageBarriers;
    };

    class PipelineBarriers
    {
    public:
        struct AccessInfo
        {
            uint64_t streamPos;
            int32_t pipelineBarrierIndex;
            VkAccessFlags accessMask;
            VkPipelineStageFlags stageMask;
        };

        struct BufferAccessInfo : AccessInfo
        {
            VkDeviceSize offset;
            VkDeviceSize range;
        };

        struct ImageAccessInfo : AccessInfo
        {
            VkImageLayout layout;
            VezImageSubresourceRange subresourceRange;
        };

        PipelineBarriers();

        std::list<PipelineBarrier>& GetBarriers() { return m_barriers; }

        std::unordered_map<Image*, ImageAccessInfo>& GetImageAccesses() { return m_imageAccesses; }

        VkImageLayout GetImageLayout(Image* pImage);

        void BufferAccess(uint64_t streamPos, Buffer* pBuffer, VkDeviceSize offset, VkDeviceSize range, VkAccessFlags accessMask, VkPipelineStageFlags stageMask);

        void ImageAccess(uint64_t streamPos, Image* pImage, const VezImageSubresourceRange* pSubresourceRange, VkImageLayout layout, VkAccessFlags accessMask, VkPipelineStageFlags stageMask);


        void Clear();

    private:
        std::unordered_map<Buffer*, BufferAccessInfo> m_bufferAccesses;
        std::unordered_map<Image*, ImageAccessInfo> m_imageAccesses;
        std::list<PipelineBarrier> m_barriers;
    };    
}