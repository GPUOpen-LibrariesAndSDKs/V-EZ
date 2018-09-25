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

#include <array>
#include <vector>
#include <list>
#include <map>
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

    /* IMPLEMENTATION NOTES:    
        This class handles tracking resource usages within the same command buffer for automated pipeline barrier insertion.
    
        Buffer accesses are tracked per region with read-combining and write-combining done on adjacent 1D ranges.
        Buffer accesses are stored in an STL map keyed by the buffer's memory address, offset and range.
    
        Image accesses are tracked per array layer and mip level ranges.  Read and write combining are performed between 2D ranges where
        the array layer is treated as the x-coordinate and mip level as the y-coordinate. If two accesses' rectangles intersect, then either
        their regions are merged into a larger rectangle or a pipeline barrier is inserted if the accesses require it.
        Images are stored in an STL unordered_map keyed by the image's memory address and value being a linked list of all accesses.
    
        This implementation likely needs to be optimized and improved to handle the cases of random scattered accesses across images and buffers as
        the process of merging and pipeline barrier insertion could become quite expensive.  However in the ideal case where accesses and linear and semi-coallesced,
        the performance should not be an issue.
    */
    class PipelineBarriers
    {
    public:
        struct AccessInfo
        {
            uint64_t streamPos;
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

        typedef std::array<uint64_t, 3> BufferAccessKey;
        typedef std::array<uint64_t, 2> ImageAccessKey;
        typedef std::list<ImageAccessInfo> ImageAccessList;

        PipelineBarriers();

        std::map<ImageAccessKey, ImageAccessList>& GetImageAccesses() { return m_imageAccesses; }

        std::list<PipelineBarrier>& GetBarriers() { return m_barriers; }

        VkImageLayout GetImageLayout(ImageView* pImageView);

        void BufferAccess(uint64_t streamPos, Buffer* pBuffer, VkDeviceSize offset, VkDeviceSize range, VkAccessFlags accessMask, VkPipelineStageFlags stageMask);

        void ImageAccess(uint64_t streamPos, Image* pImage, const VezImageSubresourceRange* pSubresourceRange, VkImageLayout layout, VkAccessFlags accessMask, VkPipelineStageFlags stageMask);

        void Clear();

    private:
        std::map<BufferAccessKey, BufferAccessInfo> m_bufferAccesses;
        std::map<ImageAccessKey, ImageAccessList> m_imageAccesses;
        std::list<PipelineBarrier> m_barriers;
    };    
}