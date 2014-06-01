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

VecLine::VecLine(double time, FILETIME systemTime, HANDLE handle, const std::string& message) :
	time(time),
	systemTime(systemTime),
	handle(handle),
	message(message)
{
}

// unused argument to allow this class to be a drop-in replacement for LineBuffer
VectorLineBuffer::VectorLineBuffer(size_t) 
{
}

void VectorLineBuffer::Add(double time, FILETIME systemTime, HANDLE handle, const char* message)
{
	boost::unique_lock<boost::mutex> lock(m_linesMutex);
	m_buffer.push_back(VecLine(time, systemTime, handle, message));
}

Lines VectorLineBuffer::GetLines()
{
	// the swap trick used here is very important to unblock the calling process asap.
	m_backingBuffer.clear();
	{
		boost::unique_lock<boost::mutex> lock(m_linesMutex);
		m_buffer.swap(m_backingBuffer);
	}
	return ProcessLines(m_backingBuffer);
}

Lines VectorLineBuffer::ProcessLines(std::vector<VecLine>& lines)
{
	Lines result;
	for (auto i = lines.begin(); i != lines.end(); ++i)
	{
		auto& line = *i;
		auto pid = GetProcessId(line.handle);
		m_handleCache.Add(pid, Handle(line.handle));

		auto processName = Str(ProcessInfo::GetProcessName(line.handle));
		result.push_back(Line(line.time, line.systemTime, pid, processName, line.message));
	}
	return result;
}

} // namespace debugviewpp
} // namespace fusion
