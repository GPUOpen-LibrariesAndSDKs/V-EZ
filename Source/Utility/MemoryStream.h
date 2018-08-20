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
#include <algorithm>
#include <iterator>
#include <list>
#include <vector>

namespace vez
{
    class MemoryStream
    {
    public:
        typedef enum SeekDir
        {
            BEG,
            END,
            CUR
        } SeekDir;

        MemoryStream(uint64_t blockSize);

        template <typename T>
        MemoryStream& operator >> (T& value)
        {
            Read(reinterpret_cast<void*>(&value), sizeof(value));
            return *this;
        }

        template <typename T>
        MemoryStream& operator << (const T& value)
        {
            Write(reinterpret_cast<const void*>(&value), sizeof(value));
            return *this;
        }

        template <typename T>
        T* ReadPtr(uint64_t count = 1)
        {
            if (m_blocks.size() == 0)
                return nullptr;

            if (m_readBlock->readAddr + sizeof(T) * count > m_readBlock->writeAddr)
            {
                if (++m_readBlock == m_blocks.end())
                    return nullptr;
                else
                    m_readBlock->readAddr = 0;
            }

            auto ptr = reinterpret_cast<T*>(&m_readBlock->allocation[m_readBlock->readAddr]);
            m_readBlock->readAddr += sizeof(T) * count;
            return ptr;
        }

        void Read(void* pData, uint64_t size);

        void Write(const void* pData, uint64_t size);

        bool EndOfStream();

        void Reset();

        void SeekG(uint64_t pos);

        void SeekG(int64_t offset, SeekDir dir);

        uint64_t TellG();

        uint64_t TellP();

        void SeekP(uint64_t pos);

        void SeekP(int64_t offset, SeekDir dir);

    private:
        struct MemoryBlock
        {
            std::vector<uint8_t> allocation;
            uint64_t readAddr;
            uint64_t writeAddr;
        };

        void AllocateNewBlock(uint64_t size);

        uint64_t m_blockSize;
        std::list<MemoryBlock> m_blocks;
        std::list<MemoryBlock>::iterator m_readBlock;
        std::list<MemoryBlock>::iterator m_writeBlock;
    };
}