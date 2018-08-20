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
#include <cstdlib>
#include <cmath>
#include "MemoryStream.h"

namespace vez
{
    MemoryStream::MemoryStream(uint64_t blockSize)
        : m_blockSize(blockSize)
    {
        AllocateNewBlock(4);
    }

    void MemoryStream::Read(void* pData, uint64_t size)
    {
        if (size == 0)
            return;

        if (m_blocks.size() == 0)
            return;

        if (m_readBlock->readAddr + size > m_readBlock->writeAddr)
        {
            if (++m_readBlock == m_blocks.end())
                return;
            else
                m_readBlock->readAddr = 0;
        }

        memcpy(pData, &m_readBlock->allocation[m_readBlock->readAddr], size);
        m_readBlock->readAddr += size;
    }

    void MemoryStream::Write(const void* pData, uint64_t size)
    {
        if (size == 0)
            return;

        AllocateNewBlock(size);

        if (m_writeBlock->writeAddr + size > m_writeBlock->allocation.size())
        {
            ++m_writeBlock;
            m_writeBlock->writeAddr = 0;
        }

        memcpy(&m_writeBlock->allocation[m_writeBlock->writeAddr], pData, size);
        m_writeBlock->writeAddr += size;
    }

    bool MemoryStream::EndOfStream()
    {
        return (m_blocks.size() == 0 || m_readBlock == m_blocks.end());
    }

    void MemoryStream::Reset()
    {
        for (auto block : m_blocks)
        {
            block.readAddr = 0;
            block.writeAddr = 0;
        }

        m_readBlock = m_blocks.begin();
        m_writeBlock = m_blocks.begin();
    }

    void MemoryStream::SeekG(uint64_t pos)
    {
        auto blockIndex = static_cast<uint32_t>(floor(pos / m_blockSize));
        blockIndex = std::min(blockIndex, static_cast<uint32_t>(m_blocks.size() - 1));
        m_readBlock = m_blocks.begin();
        std::advance(m_readBlock, blockIndex);
        m_readBlock->readAddr = pos % m_blockSize;
    }

    void MemoryStream::SeekG(int64_t offset, SeekDir dir)
    {
        if (m_blocks.size() == 0)
            return;

        switch (dir)
        {
        case BEG:
        {
            SeekG(static_cast<uint64_t>(offset));
            break;
        }

        case CUR:
        {
            if (offset > 0)
            {
                while (std::abs(offset) >= static_cast<int64_t>(m_blockSize) && m_readBlock != m_blocks.end())
                {
                    offset -= m_blockSize;
                    ++m_readBlock;
                }

                if (m_readBlock == m_blocks.end())
                {
                    --m_readBlock;
                    m_readBlock->readAddr = m_blockSize;
                }
                else
                {
                    m_readBlock->readAddr = offset;
                }
            }
            else if (offset < 0)
            {
                while (std::abs(offset) >= static_cast<int64_t>(m_blockSize) && m_readBlock != m_blocks.begin())
                {
                    offset -= m_blockSize;
                    --m_readBlock;
                }

                if (offset >= static_cast<int64_t>(m_blockSize))
                {
                    m_readBlock->readAddr = 0;
                }
                else
                {
                    m_readBlock->readAddr = offset;
                }
            }
            break;
        }

        case END:
        {
            break;
        }
        }
    }

    uint64_t MemoryStream::TellG()
    {
        if (m_blocks.size() == 0)
            return 0;

        if (m_readBlock == m_blocks.end())
            return m_blocks.size() * m_blockSize;

        return m_readBlock->readAddr + std::distance(m_blocks.begin(), m_readBlock) * m_blockSize;
    }

    uint64_t MemoryStream::TellP()
    {
        if (m_blocks.size() == 0)
            return 0;

        return m_writeBlock->writeAddr + std::distance(m_blocks.begin(), m_writeBlock) * m_blockSize;
    }

    void MemoryStream::SeekP(uint64_t pos)
    {
        auto blockIndex = static_cast<uint32_t>(floor(pos / m_blockSize));
        blockIndex = std::min(blockIndex, static_cast<uint32_t>(m_blocks.size() - 1));
        m_writeBlock = m_blocks.begin();
        std::advance(m_writeBlock, blockIndex);
        m_writeBlock->writeAddr = pos % m_blockSize;
    }

    void MemoryStream::SeekP(int64_t offset, SeekDir dir)
    {
        if (m_blocks.size() == 0)
            return;

        switch (dir)
        {
        case BEG:
        {
            SeekP(static_cast<uint64_t>(offset));
            break;
        }

        case CUR:
        {
            if (offset > 0)
            {
                while (std::abs(offset) >= static_cast<int64_t>(m_blockSize) && m_writeBlock != m_blocks.end())
                {
                    offset -= m_blockSize;
                    ++m_writeBlock;
                }

                if (m_writeBlock == m_blocks.end())
                {
                    --m_writeBlock;
                    m_writeBlock->writeAddr = m_blockSize;
                }
                else
                {
                    m_writeBlock->writeAddr = offset;
                }
            }
            else if (offset < 0)
            {
                while (std::abs(offset) >= static_cast<int64_t>(m_blockSize) && m_writeBlock != m_blocks.begin())
                {
                    offset += m_blockSize;
                    --m_writeBlock;
                }

                if (offset >= static_cast<int64_t>(m_blockSize))
                {
                    m_writeBlock->writeAddr = 0;
                }
                else
                {
                    m_writeBlock->writeAddr -= offset;
                }
            }
            break;
        }

        case END:
        {
            break;
        }
        }
    }

    void MemoryStream::AllocateNewBlock(uint64_t size)
    {
        if (m_blocks.size() == 0 || m_writeBlock->allocation.size() - m_writeBlock->writeAddr < size)
        {
            m_blocks.push_back({ std::vector<uint8_t>(m_blockSize), 0, 0 });
            m_writeBlock = m_blocks.end();
            --m_writeBlock;

            if (m_blocks.size() == 1)
                m_readBlock = m_blocks.begin();
        }
    }
}