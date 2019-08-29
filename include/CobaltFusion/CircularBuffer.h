// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <memory>
#include <string>
#include "noncopyable.h"

#pragma comment(lib, "CobaltFusion.lib")

namespace fusion {

class CircularBuffer : fusion::noncopyable
{
public:
    explicit CircularBuffer(size_t capacity);
    size_t Capacity() const;

    bool Empty() const;
    bool Full() const;
    size_t Available() const;
    size_t Size() const;

    // Performance can be improved by doing block-operations, for example using a Duff-device
    char Read();
    std::string ReadStringZ();
    void Write(char value);
    void WriteStringZ(const char* message);

    void Clear();
    void Swap(CircularBuffer& cb);
    void DumpStats() const;

private:
    size_t NextPosition(size_t offset) const;
    const char* ReadPointer() const;
    char* WritePointer() const;
    void IncreaseReadPointer();
    void IncreaseWritePointer();

    size_t m_capacity; // important: m_buffer must be initialized after m_capacity
    std::unique_ptr<char[]> m_buffer;
    size_t m_readOffset;
    size_t m_writeOffset;
};

} // namespace fusion
