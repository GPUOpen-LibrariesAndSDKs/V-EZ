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
#include "ThreadPool.h"

namespace vez
{
    ThreadPool::ThreadPool(uint32_t threadCount)
    {
        // Spawn maxThreadCount threads to execute tasks.
        for (auto i = 0U; i < threadCount; ++i)
        {
            // All threads execute until task queue becomes invalidated.
            m_threads.emplace([this](void) {
                while (true)
                {
                    // Pop the next task off of the queue.
                    std::packaged_task<void()> packagedTask;
                    if (!m_tasks.Pop(packagedTask))
                        break;

                    // Increment the number of active threads.
                    ++m_activeThreads;

                    // Run the task.
                    packagedTask();

                    // Decrement the number of active threads.
                    --m_activeThreads;

                    // Notify any thread waiting on task completition via ThreadPool::Wait().
                    m_threadsCompleteCondition.notify_all();
                }
            });
        }
    }

    ThreadPool::~ThreadPool()
    {
        // Invalidate task queue so threads stop waiting on new tasks but finish remaining tasks in queue.
        m_tasks.Invalidate();

        // Join all active threads.
        while (m_threads.size() > 0)
        {
            auto& thread = m_threads.top();
            thread.join();
            m_threads.pop();
        }
    }

    std::shared_future<void> ThreadPool::AddTask(Task&& task)
    {
        std::packaged_task<void()> packagedTask(std::move(task));
        auto future = packagedTask.get_future();
        m_tasks.Push(std::move(packagedTask));
        return future.share();
    }

    void ThreadPool::ClearPendingTasks()
    {
        m_tasks.Clear();
    }

    void ThreadPool::Wait()
    {
        std::unique_lock<std::mutex> lock(m_threadsCompleteMutex);
        m_threadsCompleteCondition.wait(lock, [this](void) { return !m_activeThreads && m_tasks.Empty(); });
    }

    void ThreadPool::Abort()
    {
        // Clear any pending items.
        m_tasks.Clear();

        // Invalidate task queue so threads stop waiting on new items.
        m_tasks.Invalidate();

        // Wait for running threads to complete.
        Wait();
    }    
}