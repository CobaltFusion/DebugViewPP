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

Lines NewlineFilter::Process(const Lines& inputlines)
{
	Lines lines;
	for (auto i = inputlines.begin(); i != inputlines.end(); ++i)
	{
		auto processedLines = Process(*i);
		for (auto i = processedLines.begin(); i != processedLines.end(); ++i)
		{
			lines.push_back(*i);
		}
	}
	FlushLinesFromTerminatedProcesses(lines);
	return lines;
}

Lines NewlineFilter::Process(const Line& line)
{
	Lines lines;
	lines.push_back(line);
	return lines;
}

void NewlineFilter::FlushLinesFromTerminatedProcesses(Lines& lines)
{
	if ((m_timer.Get() - m_handleCacheTime) < g_handleCacheTimeout)
		return;

	auto removedPIDMap = m_handleCache.CleanupMap();
	for (auto i = removedPIDMap.begin(); i != removedPIDMap.end(); i++)
	{
		DWORD pid = i->first;
		if (m_lineBuffers.find(pid) != m_lineBuffers.end())
		{
			if (!m_lineBuffers[pid].empty())
				lines.push_back(Line(m_timer.Get(), GetSystemTimeAsFileTime(), pid, "<flush>", m_lineBuffers[pid], nullptr));		// todo: messagetimestamp makes no sence, and can be out of order, maybe create a Loopback LogSource?
			lines.push_back(Line(m_timer.Get(), GetSystemTimeAsFileTime(), pid, "<terminated>", m_lineBuffers[pid], nullptr));		
			m_lineBuffers.erase(pid);
		}
	}
	m_handleCacheTime = m_timer.Get();
}

} // namespace debugviewpp 
} // namespace fusion
