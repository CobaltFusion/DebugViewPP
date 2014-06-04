// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Win32Lib/Win32Lib.h"
#include "DebugView++Lib/NewlineFilter.h"

namespace fusion {
namespace debugviewpp {

const double g_handleCacheTimeout = 15.0; //seconds

NewlineFilter::NewlineFilter() :
	m_handleCacheTime(0.0)
{
}

InputLines NewlineFilter::Process(const InputLines& inputlines)
{
	// important: flush the terminated processes before processing new lines
	// because some new lines may also be from terminated processes, but they
	// must not be flushed yet.
	auto flushedLines = FlushLinesFromTerminatedProcesses();

	InputLines lines;
	for (auto i = inputlines.begin(); i != inputlines.end(); ++i)
	{
		auto processedLines = Process(*i);
		for (auto i = processedLines.begin(); i != processedLines.end(); ++i)
		{
			lines.push_back(*i);
		}
	}
	return lines;
}

InputLines NewlineFilter::Process(const InputLine& line)
{
	InputLines lines;
	lines.push_back(line);
	return lines;
}

InputLines NewlineFilter::FlushLinesFromTerminatedProcesses()
{
	if ((m_timer.Get() - m_handleCacheTime) < g_handleCacheTimeout)
		return InputLines();

	InputLines lines;
	auto removedPIDMap = m_handleCache.CleanupMap();
	for (auto i = removedPIDMap.begin(); i != removedPIDMap.end(); i++)
	{
		DWORD pid = i->first;
		if (m_lineBuffers.find(pid) != m_lineBuffers.end())
		{
			if (!m_lineBuffers[pid].empty())
				lines.push_back(InputLine(m_timer.Get(), GetSystemTimeAsFileTime(), i->second, LineType::Flush, m_lineBuffers[pid], 0));
			m_lineBuffers.erase(pid);
		}
	}
	m_handleCacheTime = m_timer.Get();
	return lines;
}

} // namespace debugviewpp 
} // namespace fusion
