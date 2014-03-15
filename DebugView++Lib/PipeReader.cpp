// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Win32Lib/Win32Lib.h"
#include "DebugView++Lib/PipeReader.h"

namespace fusion {
namespace debugviewpp {

PipeReader::PipeReader(HANDLE hPipe, DWORD pid, const std::string& processName) :
	LogSource(SourceType::Pipe),
	m_hPipe(hPipe),
	m_pid(pid),
	m_process(processName)
{
}

HANDLE PipeReader::GetHandle() const
{
	return 0;	// todo::implement
}

void PipeReader::Notify()
{
	// add a line to CircularBuffer
}

Line PipeReader::MakeLine(const std::string& text) const
{
	Line line;
	line.time = m_timer.Get();
	line.systemTime = GetSystemTimeAsFileTime();
	line.pid = m_pid;
	line.processName = m_process;
	line.message = text;
	return line;
}

bool PipeReader::AtEnd() const
{
	return PeekNamedPipe(m_hPipe, nullptr, 0, nullptr, nullptr, nullptr) == 0;
}

Lines PipeReader::GetLines()
{
	Lines lines;
	char buf[4096];
	char* start = std::copy(m_buffer.data(), m_buffer.data() + m_buffer.size(), buf);

	DWORD avail;
	while (PeekNamedPipe(m_hPipe, nullptr, 0, nullptr, &avail, nullptr) && avail > 0)
	{
		DWORD size = buf + sizeof(buf) - start;
		DWORD read;
		ReadFile(m_hPipe, start, size, &read, nullptr);

		char* begin = buf;
		char* end = start + read;
		char* p = start;
		while (p != end)
		{
			if (*p == '\0' || *p == '\n' || p - begin > 4000)
			{
				lines.push_back(MakeLine(std::string(begin, p)));
				begin = p + 1;
			}
			++p;
		}
		start = std::copy(begin, end, buf);
	}
	m_buffer = std::string(buf, start);
	return lines;
}

} // namespace debugviewpp 
} // namespace fusion
