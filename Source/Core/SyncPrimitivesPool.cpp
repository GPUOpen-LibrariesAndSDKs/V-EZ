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
#include "Device.h"
#include "SyncPrimitivesPool.h"

namespace vez
{
    SyncPrimitivesPool::SyncPrimitivesPool(Device* device)
        : m_device(device)
    {

    }

    SyncPrimitivesPool::~SyncPrimitivesPool()
    {
        // Destroy all created fences.
        for (auto fence : m_allFences)
            vkDestroyFence(m_device->GetHandle(), fence, nullptr);

        // Destroy all created semaphores.
        for (auto semaphore : m_allSemaphores)
            vkDestroySemaphore(m_device->GetHandle(), semaphore, nullptr);
    }

    VkResult SyncPrimitivesPool::AcquireFence(VkFence* pFence)
    {
        VkResult result = VK_SUCCESS;

        // See if there's a free fence available.
        m_fenceLock.Lock();
        if (m_availableFences.size() > 0)
        {
            *pFence = m_availableFences.front();
            m_availableFences.pop();
        }
        // Else create a new one.
        else
        {
            VkFenceCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            result = vkCreateFence(m_device->GetHandle(), &createInfo, nullptr, pFence);
            if (result == VK_SUCCESS)
                m_allFences.emplace(*pFence);
        }
        m_fenceLock.Unlock();

        return result;
    }

    void SyncPrimitivesPool::ReleaseFence(VkFence fence)
    {
        m_fenceLock.Lock();
        if (m_allFences.find(fence) != m_allFences.end())
        {
            vkResetFences(m_device->GetHandle(), 1, &fence);
            m_availableFences.push(fence);
        }
        m_fenceLock.Unlock();
    }

    bool SyncPrimitivesPool::Exists(VkFence fence)
    {
        m_fenceLock.Lock();
        auto result = (m_allFences.find(fence) != m_allFences.end());
        m_fenceLock.Unlock();
        return result;
    }

    VkResult SyncPrimitivesPool::AcquireSemaphore(uint32_t semaphoreCount, VkSemaphore* pSemaphores)
    {
        VkResult result = VK_SUCCESS;

        // See if there are free semaphores available.
        m_semaphoreLock.Lock();
        while (m_availableSemaphores.size() > 0)
        {
            *pSemaphores = m_availableSemaphores.front();
            m_availableSemaphores.pop();
            ++pSemaphores;
            if (--semaphoreCount == 0)
                break;
        }

        // Create any remaining required semaphores.
        for (auto i = 0U; i < semaphoreCount; ++i)
        {
            VkSemaphoreCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            result = vkCreateSemaphore(m_device->GetHandle(), &createInfo, nullptr, &pSemaphores[i]);
            if (result != VK_SUCCESS)
                break;

            m_allSemaphores.emplace(pSemaphores[i]);
        }

        m_semaphoreLock.Unlock();
        return result;
    }

    void SyncPrimitivesPool::ReleaseSemaphores(uint32_t semaphoreCount, const VkSemaphore* pSemaphores)
    {
        m_semaphoreLock.Lock();
        for (auto i = 0U; i < semaphoreCount; ++i)
        {
            if (m_allSemaphores.find(pSemaphores[i]) != m_allSemaphores.end())
            {
                m_availableSemaphores.push(pSemaphores[i]);
            }
        }
        m_semaphoreLock.Unlock();
    }

    bool SyncPrimitivesPool::Exists(VkSemaphore semaphore)
    {
        m_semaphoreLock.Lock();
        auto result = (m_allSemaphores.find(semaphore) != m_allSemaphores.end());
        m_semaphoreLock.Unlock();
        return result;
    }
}