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
#include <cstring>
#include <iostream>
#include <algorithm>
#include "Utility/VkHelpers.h"
#include "Buffer.h"
#include "Image.h"
#include "PipelineBarriers.h"

namespace vez
{
    // Utility function for determine whether a resource access requires a pipeline barrier.
    // Returns true if a resouce is transitioning from one of the following access patterns:
    //  1. Read/write to anything
    //  2. Read to write
    //  3. Write to read
    static bool RequiresPipelineBarrier(VkAccessFlags oldAccessMask, VkAccessFlags newAccessMask)
    {
        // All read access flags.
        const VkAccessFlags allReadAccesses = VK_ACCESS_INDIRECT_COMMAND_READ_BIT
            | VK_ACCESS_INDEX_READ_BIT
            | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
            | VK_ACCESS_UNIFORM_READ_BIT
            | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT
            | VK_ACCESS_SHADER_READ_BIT
            | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
            | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
            | VK_ACCESS_TRANSFER_READ_BIT
            | VK_ACCESS_HOST_READ_BIT
            | VK_ACCESS_MEMORY_READ_BIT;

        // All write access flags.
        const VkAccessFlags allWriteAccesses = VK_ACCESS_SHADER_WRITE_BIT
            | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
            | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
            | VK_ACCESS_TRANSFER_WRITE_BIT
            | VK_ACCESS_HOST_WRITE_BIT
            | VK_ACCESS_MEMORY_WRITE_BIT;

        // Reduce accesses down to either read or write values.
        VkAccessFlags oldAccessReadWrite = 0;
        if (oldAccessMask & allReadAccesses) oldAccessReadWrite |= VK_ACCESS_MEMORY_READ_BIT;
        if (oldAccessMask & allWriteAccesses) oldAccessReadWrite |= VK_ACCESS_MEMORY_WRITE_BIT;

        VkAccessFlags newAccessReadWrite = 0;
        if (newAccessMask & allReadAccesses) newAccessReadWrite |= VK_ACCESS_MEMORY_READ_BIT;
        if (newAccessMask & allWriteAccesses) newAccessReadWrite |= VK_ACCESS_MEMORY_WRITE_BIT;

        // Handle the 3 access patterns.
        if (oldAccessReadWrite & VK_ACCESS_MEMORY_WRITE_BIT) return true;
        else if (oldAccessReadWrite != newAccessReadWrite) return true;
        else return false;
    }

    PipelineBarriers::PipelineBarriers()
    {
        // Initialize barriers array to a single entry.
        m_barriers.push_back({ 0, 0, 0, {}, {} });
    }

    VkImageLayout PipelineBarriers::GetImageLayout(Image* pImage)
    {
        auto itr = m_imageAccesses.find(pImage);
        if (itr == m_imageAccesses.end()) return pImage->GetDefaultImageLayout();
        else return itr->second.layout;
    }

    void PipelineBarriers::BufferAccess(uint64_t streamPos, Buffer* pBuffer, VkDeviceSize offset, VkDeviceSize range, VkAccessFlags accessMask, VkPipelineStageFlags stageMask)
    {
        // See if a previous access to buffer has already been stored.
        auto it = m_bufferAccesses.find(pBuffer);
        if (it != m_bufferAccesses.end())
        {
            // Check to see if old and new access patterns require a pipeline barrier.
            if (RequiresPipelineBarrier(it->second.accessMask, accessMask))
            {
                // If previous access was already added to the very last pipeline barrier, create a new pipeline barrier for proceeding accesses.
                if (it->second.pipelineBarrierIndex == static_cast<int32_t>(m_barriers.size()) - 1)
                    m_barriers.push_back({ streamPos, 0, 0,{},{} });

                // Add buffer memory barrier.
                VkBufferMemoryBarrier bufferBarrier = {};
                bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                bufferBarrier.pNext = nullptr;
                bufferBarrier.srcAccessMask = it->second.accessMask;
                bufferBarrier.dstAccessMask = accessMask;
                bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                bufferBarrier.buffer = pBuffer->GetHandle();
                bufferBarrier.offset = offset;
                bufferBarrier.size = range;

                auto& barrier = m_barriers.back();
                if (barrier.streamPosition == 0)
                    barrier.streamPosition = streamPos;

                barrier.bufferBarriers.push_back(bufferBarrier);
                barrier.srcStageMask |= it->second.stageMask;
                barrier.dstStageMask |= stageMask;

                // Update buffer access entry.
                BufferAccessInfo bufferAccessInfo = { };
                bufferAccessInfo.streamPos = streamPos;
                bufferAccessInfo.pipelineBarrierIndex = static_cast<uint32_t>(m_barriers.size() - 1);
                bufferAccessInfo.accessMask = accessMask;
                bufferAccessInfo.stageMask = stageMask;
                bufferAccessInfo.offset = offset;
                bufferAccessInfo.range = range;
                m_bufferAccesses[pBuffer] = bufferAccessInfo;
            }
            // Else combine the access and stage masks.
            else
            {
                it->second.accessMask |= accessMask;
                it->second.stageMask |= stageMask;
            }
        }
        // Else insert a new buffer access info entry.
        else
        {
            BufferAccessInfo bufferAccessInfo = {};
            bufferAccessInfo.streamPos = streamPos;
            bufferAccessInfo.pipelineBarrierIndex = -1;
            bufferAccessInfo.accessMask = accessMask;
            bufferAccessInfo.stageMask = stageMask;
            bufferAccessInfo.offset = offset;
            bufferAccessInfo.range = range;
            m_bufferAccesses.emplace(pBuffer, bufferAccessInfo);
        }
    }

    /*
        Should be factored since EVERY image layout transition requires a pipeline barrier!
        - access pattern can use the existing logic
    */
    void PipelineBarriers::ImageAccess(uint64_t streamPos, Image* pImage, const VezImageSubresourceRange* pSubresourceRange, VkImageLayout layout, VkAccessFlags accessMask, VkPipelineStageFlags stageMask)
    {
        // If image's default layout is VK_IMAGE_LAYOUT_GENERAL, keep it that way.
        //if (pImage->GetDefaultImageLayout() == VK_IMAGE_LAYOUT_GENERAL)
        //    layout = pImage->GetDefaultImageLayout();

        // See if a previous access to buffer has already been stored.
        auto it = m_imageAccesses.find(pImage);
        if (it != m_imageAccesses.end())
        {
            // Check to see if old and new access masks or image layouts require a pipeline barrier.
            if (RequiresPipelineBarrier(it->second.accessMask, accessMask) || it->second.layout != layout)
            {
                // If previous access was already added to the very last pipeline barrier, create a new pipeline barrier for proceeding accesses.
                if (it->second.pipelineBarrierIndex == static_cast<int32_t>(m_barriers.size()) - 1)
                    m_barriers.push_back({ streamPos, 0, 0,{},{} });

                // Add image memory barrier.
                // TODO! - take into account miplevel and layer for pipeline barrier.
                //       - code should not necessarily insert a pipeline barrier if different layers or mip levels have different access patterns.
                VkImageMemoryBarrier imageBarrier = {};
                imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                imageBarrier.srcAccessMask = static_cast<VkAccessFlags>(it->second.accessMask);
                imageBarrier.dstAccessMask = static_cast<VkAccessFlags>(accessMask);
                imageBarrier.oldLayout = it->second.layout;
                imageBarrier.newLayout = layout;
                imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

                imageBarrier.image = pImage->GetHandle();
                imageBarrier.subresourceRange.aspectMask = GetImageAspectFlags(pImage->GetCreateInfo().format);
                imageBarrier.subresourceRange.baseMipLevel = pSubresourceRange->baseMipLevel;
                imageBarrier.subresourceRange.levelCount = pSubresourceRange->levelCount;
                imageBarrier.subresourceRange.baseArrayLayer = pSubresourceRange->baseArrayLayer;
                imageBarrier.subresourceRange.layerCount = pSubresourceRange->layerCount;

                auto& barrier = m_barriers.back();
                if (barrier.streamPosition == 0)
                    barrier.streamPosition = streamPos;

                barrier.imageBarriers.push_back(imageBarrier);
                barrier.srcStageMask |= it->second.stageMask;
                barrier.dstStageMask |= stageMask;

                // Update image access entry.
                ImageAccessInfo imageAccessInfo = {};
                imageAccessInfo.streamPos = streamPos;
                imageAccessInfo.pipelineBarrierIndex = static_cast<uint32_t>(m_barriers.size() - 1);
                imageAccessInfo.accessMask = accessMask;
                imageAccessInfo.stageMask = stageMask;
                imageAccessInfo.layout = layout;
                memcpy(&imageAccessInfo.subresourceRange, pSubresourceRange, sizeof(VezImageSubresourceRange));
                m_imageAccesses[pImage] = imageAccessInfo;
            }
            // Else combine the access, stage masks and subresource range.
            else
            {
                it->second.accessMask |= accessMask;
                it->second.stageMask |= stageMask;

                auto maxMipLevel = std::max(it->second.subresourceRange.baseMipLevel + it->second.subresourceRange.levelCount, pSubresourceRange->baseMipLevel + pSubresourceRange->levelCount);
                auto maxLayer = std::max(it->second.subresourceRange.baseArrayLayer + it->second.subresourceRange.layerCount, pSubresourceRange->baseArrayLayer + pSubresourceRange->layerCount);
                it->second.subresourceRange.baseMipLevel = std::min(it->second.subresourceRange.baseMipLevel, pSubresourceRange->baseMipLevel);
                it->second.subresourceRange.levelCount = maxMipLevel - it->second.subresourceRange.baseMipLevel;
                it->second.subresourceRange.baseArrayLayer = std::min(it->second.subresourceRange.baseArrayLayer, pSubresourceRange->baseArrayLayer);
                it->second.subresourceRange.layerCount = maxLayer - it->second.subresourceRange.baseArrayLayer;
            }
        }
        // Else check to see if new layout differs from default layout and insert a new image access info entry.
        else
        {
            // Add image memory barrier if new layout differs from old layout.
            auto defaultLayout = pImage->GetDefaultImageLayout();
            auto pipelineBarrierIndex = -1;
            if (defaultLayout != layout)
            {
                VkImageMemoryBarrier imageBarrier = {};
                imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                imageBarrier.srcAccessMask = 0;
                imageBarrier.dstAccessMask = static_cast<VkAccessFlags>(accessMask);
                imageBarrier.oldLayout = defaultLayout;
                imageBarrier.newLayout = layout;
                imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

                imageBarrier.image = pImage->GetHandle();
                imageBarrier.subresourceRange.aspectMask = GetImageAspectFlags(pImage->GetCreateInfo().format);
                imageBarrier.subresourceRange.baseMipLevel = pSubresourceRange->baseMipLevel;
                imageBarrier.subresourceRange.levelCount = pSubresourceRange->levelCount;
                imageBarrier.subresourceRange.baseArrayLayer = pSubresourceRange->baseArrayLayer;
                imageBarrier.subresourceRange.layerCount = pSubresourceRange->layerCount;

                //m_barriers.push_back({ streamPos, 0, 0, {}, {} });
                auto& barrier = m_barriers.back();
                if (barrier.streamPosition == 0)
                    barrier.streamPosition = streamPos;

                barrier.imageBarriers.push_back(imageBarrier);
                barrier.srcStageMask |= stageMask;
                barrier.dstStageMask |= stageMask;

                pipelineBarrierIndex = static_cast<int>(m_barriers.size()) - 1;
            }

            // Add entry to image accesses.
            ImageAccessInfo imageAccessInfo = {};
            imageAccessInfo.streamPos = streamPos;
            imageAccessInfo.pipelineBarrierIndex = pipelineBarrierIndex;
            imageAccessInfo.accessMask = accessMask;
            imageAccessInfo.stageMask = stageMask;
            imageAccessInfo.layout = layout;
            memcpy(&imageAccessInfo.subresourceRange, pSubresourceRange, sizeof(VezImageSubresourceRange));
            m_imageAccesses.emplace(pImage, imageAccessInfo);
        }
    }

    void PipelineBarriers::Clear()
    {
        // Clear internal arrays.
        m_bufferAccesses.clear();
        m_imageAccesses.clear();
        m_barriers.clear();

        // Insert single entry into barriers.
        m_barriers.push_back({ 0, 0, 0,{},{} });
    }    
}