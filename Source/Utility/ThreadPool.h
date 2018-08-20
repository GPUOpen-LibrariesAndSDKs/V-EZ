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

#include <stdint.h>
#include <assert.h>
#include <stack>
#include <queue>
#include <future>
#include <atomic>
#include <condition_variable>
#include <mutex>

namespace vez
{
    // A thread-safe queue class.
    template <typename T>
    class ThreadSafeQueue
    {
    public:
        bool Empty()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_queue.empty();
        }

        void Clear()
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            while (!m_queue.empty())
                m_queue.pop();
            m_condition.notify_all();
        }

        void Invalidate()
        {
            m_valid = false;
            m_condition.notify_all();
        }

        void Push(const T& item)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            assert(m_valid);
            m_queue.push(item);
            lock.unlock();
            m_condition.notify_one();
        }

        void Push(T&& item)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            assert(m_valid);
            m_queue.push(std::move(item));
            lock.unlock();
            m_condition.notify_one();
        }

        bool Pop(T& item)
        {
            // Wait for an item to be in the queue or the queue to be invalidated.
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.wait(lock, [this](void) { return !m_queue.empty() || !m_valid; });

            // Ensure queue is still valid since above predicate could fall through.
            if (!m_valid)
                return false;

            // Get the item out of the queue.
            item = std::move(m_queue.front());
            m_queue.pop();
            return true;
        }

    private:
        std::atomic_bool m_valid { true };
        std::queue<T> m_queue;
        std::mutex m_mutex;
        std::condition_variable m_condition;
    };

    // A ThreadPool class for parallel executions of tasks on across one or more threads.
    class ThreadPool
    {
    public:
        // Task definition for code simplicity purposes.
        using Task = std::function<void()>;

        // Constructs the thread pool and sets the max thread count.
        // No threads are allocated until tasks are added to the queue.
        ThreadPool(uint32_t threadCount);

        // Blocks until all threads have completed.
        ~ThreadPool();

        // Adds a new task to the thread pool and returns a future handle.
        std::shared_future<void> AddTask(Task&& task);

        // Clear any pending tasks.
        void ClearPendingTasks();

        // Waits on all threads to complete (blocking call).
        void Wait();

        // Cancels all pending tasks and waits for threads to complete current tasks.
        void Abort();

    private:
        std::stack<std::thread> m_threads;
        ThreadSafeQueue<std::packaged_task<void()>> m_tasks;
        std::atomic<uint32_t> m_activeThreads { 0U };
        std::mutex m_threadsCompleteMutex;
        std::condition_variable m_threadsCompleteCondition;
    };    
}