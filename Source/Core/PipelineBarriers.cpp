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
#include "ImageView.h"
#include "PipelineBarriers.h"

namespace vez
{
    // Utility function for determine whether a resource access requires a pipeline barrier.
    // Returns true if a resource is transitioning from one of the following access patterns:
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

    }

    VkImageLayout PipelineBarriers::GetImageLayout(ImageView* pImageView)
    {
        // If an access entry exists for the image, find the one at the specified base mip level.
        auto key = ImageAccessKey{ reinterpret_cast<uint64_t>(pImageView->GetImage()), static_cast<uint64_t>(pImageView->GetSubresourceRange().baseArrayLayer) };
        auto entry = m_imageAccesses.find(key);
        if (entry != m_imageAccesses.end())
        {
            auto& accessList = entry->second;
            for (auto& access : accessList)
            {
                if (access.subresourceRange.baseMipLevel == pImageView->GetSubresourceRange().baseMipLevel)
                    return access.layout;
            }
        }
        
        // Else return the default layout.
        return pImageView->GetImage()->GetDefaultImageLayout();
    }

    void PipelineBarriers::BufferAccess(uint64_t streamPos, Buffer* pBuffer, VkDeviceSize offset, VkDeviceSize range, VkAccessFlags accessMask, VkPipelineStageFlags stageMask)
    {
        // Insert new entry into buffer accesses map.
        BufferAccessInfo bufferAccessInfo = {};
        bufferAccessInfo.streamPos = streamPos;
        bufferAccessInfo.accessMask = accessMask;
        bufferAccessInfo.stageMask = stageMask;
        bufferAccessInfo.offset = offset;
        bufferAccessInfo.range = range;

        auto insertKey = BufferAccessKey{ reinterpret_cast<uint64_t>(pBuffer), offset, range };
        auto result = m_bufferAccesses.emplace(insertKey, bufferAccessInfo);

        // If entry successfully inserted (i.e. key did not exist) then combine with previous entries.
        if (std::get<1>(result) == true)
        {
            // Iterate over previous and proceeding entries that overlap new access.
            auto finalKey = insertKey;
            auto iter = std::get<0>(result); --iter;
            bool combinedEntries = false;
            bool insertPipelineBarrier = false;
            bool createNewPipelineBarrierEntry = false;
            VkAccessFlags oldAccessMask = 0;
            VkPipelineStageFlags oldStageMask = 0;
            while (iter != m_bufferAccesses.end() && iter->first[0] == insertKey[0])
            {
                // Skip entry that was just inserted.
                if (iter->first == insertKey)
                {
                    ++iter;
                    continue;
                }

                // Check to see if previous access overlaps with new access.
                auto min = std::min(finalKey[1], iter->first[1]);
                auto max = std::max(finalKey[1] + finalKey[2], iter->first[1] + iter->first[2]);
                if (max - min < finalKey[2] + iter->first[2])
                {
                    // Check to see if new access requires a pipeline barrier.
                    if (RequiresPipelineBarrier(iter->second.accessMask, accessMask))
                    {
                        // Set flags, merge accesses and delete old access entry.
                        insertPipelineBarrier = true;
                        createNewPipelineBarrierEntry = true;
                        oldAccessMask |= iter->second.accessMask;
                        oldStageMask |= iter->second.stageMask;
                        iter = m_bufferAccesses.erase(iter);
                    }
                    else
                    {
                        // Combine old entry with new entry.
                        auto newOffset = std::min(finalKey[1], iter->first[1]);
                        auto newSize = std::max(finalKey[1] + finalKey[2], iter->first[1] + iter->first[2]) - newOffset;
                        finalKey[1] = newOffset;
                        finalKey[2] = newSize;
                        accessMask |= iter->second.accessMask;
                        stageMask |= iter->second.stageMask;
                        iter = m_bufferAccesses.erase(iter);
                        combinedEntries = true;
                    }
                }
                // Else move to next entry.
                else
                {
                    ++iter;
                }
            }

            // If entries were combined with new one, remove new entry and reinsert it with new key.
            if (combinedEntries)
            {
                m_bufferAccesses.erase(insertKey);

                bufferAccessInfo.accessMask = accessMask;
                bufferAccessInfo.stageMask = stageMask;
                bufferAccessInfo.offset = offset;
                bufferAccessInfo.range = range;
                m_bufferAccesses.emplace(finalKey, bufferAccessInfo);
            }

            // Add pipeline barrier if required.
            if (insertPipelineBarrier)
            {
                // Add a new entry if any of the erased entries were already assigned to a pipeline barrier index.
                //if (createNewPipelineBarrierEntry)
                //    m_barriers.push_back({ streamPos, 0, 0, {}, {} });
                // Create first barrier entry or merge with previous barrier if access is at same stream position.
                if (m_barriers.size() == 0 || m_barriers.back().streamPosition != streamPos)
                    m_barriers.push_back({ streamPos, 0, 0, {}, {} });

                // Add the new buffer memory barrier.
                VkBufferMemoryBarrier bufferBarrier = {};
                bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                bufferBarrier.pNext = nullptr;
                bufferBarrier.srcAccessMask = oldAccessMask;
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
                barrier.srcStageMask |= oldStageMask;
                barrier.dstStageMask |= stageMask;

                // Update buffer access entry.
                auto& bufferAccessInfo = m_bufferAccesses.at(finalKey);
                bufferAccessInfo.streamPos = streamPos;
                bufferAccessInfo.accessMask = accessMask;
                bufferAccessInfo.stageMask = stageMask;
                bufferAccessInfo.offset = offset;
                bufferAccessInfo.range = range;
            }
        }
        // A previous access entry exist with the same offset and range.  Check to see if a pipeline barrier is required.
        else if (RequiresPipelineBarrier(std::get<0>(result)->second.accessMask, accessMask))
        {
            // Create first barrier entry or merge with previous barrier if access is at same stream position.
            if (m_barriers.size() == 0 || m_barriers.back().streamPosition != streamPos)
                m_barriers.push_back({ streamPos, 0, 0,{},{} });

            // Add buffer memory barrier.
            auto& prevEntry = std::get<0>(result)->second;
            VkBufferMemoryBarrier bufferBarrier = {};
            bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            bufferBarrier.pNext = nullptr;
            bufferBarrier.srcAccessMask = prevEntry.accessMask;
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
            barrier.srcStageMask |= prevEntry.stageMask;
            barrier.dstStageMask |= stageMask;

            // Update buffer access entry.
            BufferAccessInfo bufferAccessInfo = {};
            bufferAccessInfo.streamPos = streamPos;
            bufferAccessInfo.accessMask = accessMask;
            bufferAccessInfo.stageMask = stageMask;
            bufferAccessInfo.offset = offset;
            bufferAccessInfo.range = range;
            m_bufferAccesses.erase(std::get<0>(result));
            m_bufferAccesses.emplace(insertKey, bufferAccessInfo);
        }
    }

    void PipelineBarriers::ImageAccess(uint64_t streamPos, Image* pImage, const VezImageSubresourceRange* pSubresourceRange, VkImageLayout layout, VkAccessFlags accessMask, VkPipelineStageFlags stageMask)
    {
        // Per image accesses per layer are stored/merged independently.
        auto layerCount = pSubresourceRange->layerCount;
        if (layerCount == VK_REMAINING_ARRAY_LAYERS)
            layerCount = pImage->GetCreateInfo().arrayLayers - pSubresourceRange->baseArrayLayer;

        for (auto layer = pSubresourceRange->baseArrayLayer; layer < pSubresourceRange->baseArrayLayer + layerCount; ++layer)
        {
            // Check if a previous access to buffer has already been stored.  If none exists, insert entry with empty access list.
            auto key = ImageAccessKey{ reinterpret_cast<uint64_t>(pImage), static_cast<uint64_t>(layer) };
            auto entry = m_imageAccesses.find(key);
            if (entry != m_imageAccesses.end())
            {
                // Mip level access bit mask.
                auto levelCount = pSubresourceRange->levelCount;
                if (levelCount == VK_REMAINING_MIP_LEVELS)
                    levelCount = pImage->GetCreateInfo().mipLevels - pSubresourceRange->baseMipLevel;

                auto mipLevelAccessMask = ((1U << levelCount) - 1U) << pSubresourceRange->baseMipLevel;
                auto mipLevelBarrierMask = mipLevelAccessMask;

                // List of new accesses to add after iteration (old accesses that were split).
                std::list<ImageAccessInfo> splitAccesses;

                // Iterate over list of previous accesses.
                auto& accessList = entry->second;
                auto iter = accessList.begin();
                while (iter != accessList.end())
                {
                    // If no bits are currently set in mip level access, quit iterating.
                    if (!mipLevelAccessMask)
                        break;

                    // Check to see if new access requires a pipeline barrier based on the old access if or image layout is changing.
                    if (RequiresPipelineBarrier(iter->accessMask, accessMask) || iter->layout != layout)
                    {
                        // Check to see if previous and new access overlap across mip level range.
                        auto min = std::min(pSubresourceRange->baseMipLevel, iter->subresourceRange.baseMipLevel);
                        auto max = std::max(pSubresourceRange->baseMipLevel + levelCount, iter->subresourceRange.baseMipLevel + iter->subresourceRange.levelCount);
                        if (max - min < levelCount + iter->subresourceRange.levelCount)
                        {
                            // Calculate min and max mip level range for intersection.
                            auto baseMipLevelMin = std::max(iter->subresourceRange.baseMipLevel, pSubresourceRange->baseMipLevel);
                            auto baseMipLevelMax = std::min(iter->subresourceRange.baseMipLevel + iter->subresourceRange.levelCount, pSubresourceRange->baseMipLevel + levelCount);
                            auto levelCount = baseMipLevelMax - baseMipLevelMin;

                            // Try and merge barriers if they're occuring at the same command stream position.  Else create a new barrier entry.
                            if (m_barriers.size() == 0 || m_barriers.back().streamPosition != streamPos)
                                m_barriers.push_back({ streamPos, iter->stageMask, stageMask, {}, {} });

                            // Add image memory barrier for overlapping region.
                            VkImageMemoryBarrier imageBarrier = {};
                            imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                            imageBarrier.srcAccessMask = iter->accessMask;;
                            imageBarrier.dstAccessMask = accessMask;
                            imageBarrier.oldLayout = iter->layout;
                            imageBarrier.newLayout = layout;
                            imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                            imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                            imageBarrier.image = pImage->GetHandle();
                            imageBarrier.subresourceRange.aspectMask = GetImageAspectFlags(pImage->GetCreateInfo().format);
                            imageBarrier.subresourceRange.baseMipLevel = baseMipLevelMin;
                            imageBarrier.subresourceRange.levelCount = levelCount;
                            imageBarrier.subresourceRange.baseArrayLayer = layer;
                            imageBarrier.subresourceRange.layerCount = 1;
                            m_barriers.back().imageBarriers.push_back(imageBarrier);

                            // Shrink and/or split original entry.
                            if (iter->subresourceRange.baseMipLevel < baseMipLevelMin && iter->subresourceRange.baseMipLevel + iter->subresourceRange.levelCount > baseMipLevelMax)
                            {
                                // Shrink original entry.
                                iter->subresourceRange.levelCount = imageBarrier.subresourceRange.baseMipLevel - iter->subresourceRange.baseMipLevel;

                                // Add a new entry for remaining tail end of previous access.
                                ImageAccessInfo newAccessInfo = *iter;
                                newAccessInfo.subresourceRange.baseMipLevel = baseMipLevelMax;
                                newAccessInfo.subresourceRange.levelCount = (iter->subresourceRange.baseMipLevel + iter->subresourceRange.levelCount) - baseMipLevelMax;
                                splitAccesses.push_back(newAccessInfo);

                                // Move to next entry.
                                ++iter;
                            }
                            // Old entry is split at the front.
                            else if (iter->subresourceRange.baseMipLevel < baseMipLevelMin)
                            {
                                // Shrink original entry.
                                iter->subresourceRange.levelCount = imageBarrier.subresourceRange.baseMipLevel - iter->subresourceRange.baseMipLevel;

                                // Move to next entry.
                                ++iter;
                            }
                            // Old entry is split at the tail.
                            else if (iter->subresourceRange.baseMipLevel + iter->subresourceRange.levelCount > baseMipLevelMax)
                            {
                                // Shrink and adjust base mip level of original entry.
                                iter->subresourceRange.levelCount = (iter->subresourceRange.baseMipLevel + iter->subresourceRange.levelCount) - baseMipLevelMax;
                                iter->subresourceRange.baseMipLevel = baseMipLevelMax;

                                // Move to next entry.
                                ++iter;
                            }
                            // Else remove original entry.
                            else
                            {
                                iter = accessList.erase(iter);
                            }

                            // Unset relevant bits in mip level access and barrier masks.
                            auto unsetBits = (((1U << levelCount) - 1U) << baseMipLevelMin);
                            //mipLevelAccessMask &= ~unsetBits;
                            mipLevelBarrierMask &= ~unsetBits;
                        }
                        // If they don't overlap, no action needs to be taken.  Move onto next entry.
                        else
                        {
                            ++iter;
                        }
                    }
                    // See if old entry can be removed.
                    else
                    {
                        // See if old and new access can be merged.
                        auto expandedMipLevelAccessMask = mipLevelAccessMask | (mipLevelAccessMask << 1) | (mipLevelAccessMask >> 1);
                        auto prevAccessMipLevelAccessMask = ((1U << iter->subresourceRange.levelCount) - 1U) << iter->subresourceRange.baseMipLevel;
                        if ((prevAccessMipLevelAccessMask & expandedMipLevelAccessMask) != 0 && iter->layout == layout)
                        {
                            // Combine old access into new access.
                            mipLevelAccessMask |= prevAccessMipLevelAccessMask;

                            // Remove old access entry.
                            iter = accessList.erase(iter);
                        }
                        // Else move onto next entry.
                        else
                        {
                            ++iter;
                        }
                    }
                }

                // Add all old accesses that were split back into access list.
                accessList.insert(accessList.begin(), splitAccesses.begin(), splitAccesses.end());

                // Add new entries for remaining active bits in access mask.
                while (mipLevelAccessMask)
                {
                    // Determine start and end of next bit range.
                    auto bitOffsetStart = FindLSB(mipLevelAccessMask);
                    auto bitOffsetEnd = FindLSB(~mipLevelAccessMask & ~((1U << bitOffsetStart) - 1U));
                    auto length = bitOffsetEnd - bitOffsetStart;

                    // Remove bits from mask.
                    mipLevelAccessMask &= ~((1U << bitOffsetEnd) - 1U);

                    // Add new access entry.
                    ImageAccessInfo access = {};
                    access.streamPos = streamPos;
                    access.accessMask = accessMask;
                    access.stageMask = stageMask;
                    access.layout = layout;
                    access.subresourceRange.baseMipLevel = bitOffsetStart;
                    access.subresourceRange.levelCount = length;
                    access.subresourceRange.baseArrayLayer = layer;
                    access.subresourceRange.layerCount = 1;
                    accessList.push_back(access);
                }

                // If current access is changing the default image layout, add one or more image memory barriers.
                if (pImage->GetDefaultImageLayout() != layout && mipLevelBarrierMask)
                {
                    // Create first barrier entry or merge with previous barrier if access is at same stream position.
                    if (m_barriers.size() == 0 || m_barriers.back().streamPosition != streamPos)
                        m_barriers.push_back({ streamPos, stageMask, stageMask, {}, {} });

                    // Insert an image memory barrier for each group of active bits in mip level barrier mask.
                    while (mipLevelBarrierMask)
                    {
                        // Determine start and end of next bit range.
                        auto bitOffsetStart = FindLSB(mipLevelBarrierMask);
                        auto bitOffsetEnd = FindLSB(~mipLevelBarrierMask & ~((1U << bitOffsetStart) - 1U));
                        auto length = bitOffsetEnd - bitOffsetStart;

                        // Remove bits from mask.
                        mipLevelBarrierMask &= ~((1U << bitOffsetEnd) - 1U);

                        VkImageMemoryBarrier imageBarrier = {};
                        imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                        imageBarrier.srcAccessMask = 0;
                        imageBarrier.dstAccessMask = accessMask;
                        imageBarrier.oldLayout = pImage->GetDefaultImageLayout();
                        imageBarrier.newLayout = layout;
                        imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        imageBarrier.image = pImage->GetHandle();
                        imageBarrier.subresourceRange.aspectMask = GetImageAspectFlags(pImage->GetCreateInfo().format);
                        imageBarrier.subresourceRange.baseMipLevel = bitOffsetStart;
                        imageBarrier.subresourceRange.levelCount = length;
                        imageBarrier.subresourceRange.baseArrayLayer = layer;
                        imageBarrier.subresourceRange.layerCount = 1;

                        m_barriers.back().dstStageMask |= stageMask;
                        m_barriers.back().imageBarriers.push_back(imageBarrier);
                    }
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
                    // Create first barrier entry or merge with previous barrier if access is at same stream position.
                    if (m_barriers.size() == 0 || m_barriers.back().streamPosition != streamPos)
                        m_barriers.push_back({ streamPos, stageMask, stageMask, {}, {} });

                    VkImageMemoryBarrier imageBarrier = {};
                    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    imageBarrier.srcAccessMask = 0;
                    imageBarrier.dstAccessMask = accessMask;
                    imageBarrier.oldLayout = defaultLayout;
                    imageBarrier.newLayout = layout;
                    imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    imageBarrier.image = pImage->GetHandle();
                    imageBarrier.subresourceRange.aspectMask = GetImageAspectFlags(pImage->GetCreateInfo().format);
                    imageBarrier.subresourceRange.baseMipLevel = pSubresourceRange->baseMipLevel;
                    imageBarrier.subresourceRange.levelCount = pSubresourceRange->levelCount;
                    imageBarrier.subresourceRange.baseArrayLayer = layer;
                    imageBarrier.subresourceRange.layerCount = 1;

                    m_barriers.back().dstStageMask |= stageMask;
                    m_barriers.back().imageBarriers.push_back(imageBarrier);
                }

                // Add entry to image accesses.
                ImageAccessInfo imageAccessInfo = {};
                imageAccessInfo.streamPos = streamPos;
                imageAccessInfo.accessMask = accessMask;
                imageAccessInfo.stageMask = stageMask;
                imageAccessInfo.layout = layout;
                imageAccessInfo.subresourceRange.baseArrayLayer = layer;
                imageAccessInfo.subresourceRange.layerCount = 1;
                imageAccessInfo.subresourceRange.baseMipLevel = pSubresourceRange->baseMipLevel;
                imageAccessInfo.subresourceRange.levelCount = pSubresourceRange->levelCount;
                if (imageAccessInfo.subresourceRange.levelCount == VK_REMAINING_MIP_LEVELS)
                    imageAccessInfo.subresourceRange.levelCount = pImage->GetCreateInfo().mipLevels - pSubresourceRange->baseMipLevel;

                ImageAccessList accessList = { imageAccessInfo };
                m_imageAccesses.emplace(key, std::move(accessList));
            }
        }     
    }

    void PipelineBarriers::Clear()
    {
        // Clear internal arrays.
        m_bufferAccesses.clear();
        m_imageAccesses.clear();
        m_barriers.clear();
    }    
}