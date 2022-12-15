// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include "CobaltFusion/CircularBuffer.h"
#include "CobaltFusion/dbgstream.h"

namespace fusion {

// see http://en.wikipedia.org/wiki/Circular_buffer
// the 'Always Keep One Slot Open' strategy is used to distigush between empty and full conditions

// +---+---+---+---+---+---+---+---+
// | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |    Empty Buffer
// +---+----+--+---+---+---+---+---+
//               R
//               W

// +---+---+---+---+---+---+---+---+
// | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |    1 Bytes in buffer
// +---+----+--+---+---+---+---+---+
//               R   W
//

// +---+---+---+---+---+---+---+---+
// | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |    2 Bytes in buffer
// +---+----+--+---+---+---+---+---+
//               R       W
//

// +---+---+---+---+---+---+---+---+
// | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |    2 Bytes in buffer
// +---+----+--+---+---+---+---+---+
//       W                       R
//

// +---+---+---+---+---+---+---+---+
// | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |    Full buffer (1 byte used as 'open slot')
// +---+---+---+---+---+---+---+---+
//           W   R

// +---+---+---+---+---+---+---+---+
// | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |    Full buffer (1 byte used as 'open slot')
// +---+---+---+---+---+---+---+---+
//   R                           W


CircularBuffer::CircularBuffer(size_t capacity) :
    m_capacity(capacity),
    m_buffer(new char[capacity + 1]),
    m_readOffset(0),
    m_writeOffset(0)
{
}

size_t CircularBuffer::Capacity() const
{
    return m_capacity;
}

bool CircularBuffer::Empty() const
{
    return m_readOffset == m_writeOffset;
}

size_t CircularBuffer::Available() const
{
    return m_writeOffset < m_readOffset ? m_readOffset - m_writeOffset - 1 : m_capacity - m_writeOffset + m_readOffset;
}

size_t CircularBuffer::Size() const
{
    return m_writeOffset < m_readOffset ? m_capacity + 1 - m_readOffset + m_writeOffset : m_writeOffset - m_readOffset;
}

bool CircularBuffer::Full() const
{
    //std::cerr << "full: " << NextPosition(m_writeOffset) << " ?= " << m_readOffset << "\n"
    return NextPosition(m_writeOffset) == m_readOffset; // actually full
}

std::string CircularBuffer::ReadStringZ()
{
    //std::cerr << "  ReadStringZ\n";

    std::string message;
    while (auto ch = Read())
    {
        message.push_back(ch);
    }
    //std::cerr << "  ReadStringZ done\n";
    return message;
}

void CircularBuffer::WriteStringZ(const char* message)
{
    //std::cerr << "  WriteStringZ\n";
    while (*message != 0)
    {
        Write(*message);
        ++message;
    }
    Write('\0');
    //std::cerr << "  WriteStringZ done\n";
}

size_t CircularBuffer::NextPosition(size_t offset) const
{
    return offset == m_capacity ? 0 : offset + 1;
}

const char* CircularBuffer::ReadPointer() const
{
    return m_buffer.get() + m_readOffset;
}

char* CircularBuffer::WritePointer() const
{
    return m_buffer.get() + m_writeOffset;
}

void CircularBuffer::IncreaseReadPointer()
{
    m_readOffset = NextPosition(m_readOffset);
}

void CircularBuffer::IncreaseWritePointer()
{
    m_writeOffset = NextPosition(m_writeOffset);
}

void CircularBuffer::Clear()
{
    m_readOffset = 0;
    m_writeOffset = 0;
}

void CircularBuffer::Swap(CircularBuffer& cb)
{
    std::swap(m_capacity, cb.m_capacity);
    std::swap(m_buffer, cb.m_buffer);
    std::swap(m_readOffset, cb.m_readOffset);
    std::swap(m_writeOffset, cb.m_writeOffset);
}

char CircularBuffer::Read()
{
    if (Empty())
    {
        throw std::exception("Read from empty buffer!");
    }

    auto value = *ReadPointer();
    //std::cerr << "  " << m_readOffset << " => " << unsigned int(unsigned char(value)) << "\n";
    IncreaseReadPointer();
    return value;
}

void CircularBuffer::Write(char value)
{
    //std::cerr << "  " << m_writeOffset << " <= " << unsigned int(unsigned char(value)) << "\n";
    *WritePointer() = value;
    IncreaseWritePointer();
}

void CircularBuffer::DumpStats() const
{
    std::cerr << "  m_readOffset:  " << m_readOffset << "\n";
    std::cerr << "  m_writeOffset: " << m_writeOffset << "\n";
    std::cerr << "  Empty: " << (Empty() ? "true" : "false") << "\n";
    std::cerr << "  Full:  " << (Full() ? "true" : "false") << "\n";
}

} // namespace fusion
