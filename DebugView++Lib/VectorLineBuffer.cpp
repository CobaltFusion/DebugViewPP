// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "stdafx.h"
#include "DebugView++Lib/VectorLineBuffer.h"
#include "DebugView++Lib/ProcessInfo.h"
#include "Win32Lib/utilities.h"

namespace fusion {
namespace debugviewpp {

// unused argument to allow this class to be a drop-in replacement for LineBuffer
VectorLineBuffer::VectorLineBuffer(size_t) 
{
}

void VectorLineBuffer::Add(double time, FILETIME systemTime, HANDLE handle, const char* message, std::shared_ptr<LogSource> logsource)
{
	boost::unique_lock<boost::mutex> lock(m_linesMutex);
	m_buffer.push_back(Line(time, systemTime, handle, message, logsource));
}

void VectorLineBuffer::Add(double time, FILETIME systemTime, DWORD pid, const char* processName, const char* message, std::shared_ptr<LogSource> logsource)
{
	boost::unique_lock<boost::mutex> lock(m_linesMutex);
	m_buffer.push_back(Line(time, systemTime, pid, processName, message, logsource));
}

Lines VectorLineBuffer::GetLines()
{
	// the swap trick used here is very important to unblock the calling process asap.
	m_backingBuffer.clear();
	{
		boost::unique_lock<boost::mutex> lock(m_linesMutex);
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
