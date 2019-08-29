// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DebugView++Lib/ProcessInfo.h"
#include "DebugView++Lib/VectorLineBuffer.h"

namespace fusion {
namespace debugviewpp {

// unused argument to allow this class to be a drop-in replacement for LineBuffer
VectorLineBuffer::VectorLineBuffer(size_t /*unused*/)
{
}

void VectorLineBuffer::Add(double time, FILETIME systemTime, HANDLE handle, const std::string& message, const LogSource* pSource)
{
    std::lock_guard<std::mutex> lock(m_linesMutex);
    m_buffer.emplace_back(time, systemTime, handle, message, pSource);
}

void VectorLineBuffer::Add(double time, FILETIME systemTime, DWORD pid, const std::string& processName, const std::string& message, const LogSource* pSource)
{
    std::lock_guard<std::mutex> lock(m_linesMutex);
    m_buffer.emplace_back(time, systemTime, pid, processName, message, pSource);
}

// returning a 'const Lines&' here might be an performance improvement, however, tests reveiled no measureable difference.
Lines VectorLineBuffer::GetLines()
{
    // the swap trick used here is very important to unblock the calling process asap.
    m_backingBuffer.clear();
    {
        std::lock_guard<std::mutex> lock(m_linesMutex);
        m_buffer.swap(m_backingBuffer);
    }
    return m_backingBuffer;
}

bool VectorLineBuffer::Empty() const
{
    return m_buffer.empty();
}

} // namespace debugviewpp
} // namespace fusion
