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

NewlineFilter::NewlineFilter()
{
}

Lines NewlineFilter::Process(const Line& line)
{
	Lines lines;
	if (m_lineBuffers.find(line.pid) == m_lineBuffers.end())
	{
		std::string message;
		message.reserve(4000);
		m_lineBuffers[line.pid] = std::move(message);
	}
	std::string& message = m_lineBuffers[line.pid];

	Line outputLine = line;
	for (auto i = line.message.begin(); i != line.message.end(); i++)
	{
		if (*i == '\r')
			continue;

		if (*i == '\n')
		{
			outputLine.message = message;
			message.clear();
			lines.push_back(outputLine);
		}
		else
		{
			message.push_back(char(*i));
		}
	}

	if (message.empty())
	{
		m_lineBuffers.erase(line.pid);
	}
	else if (message.size() > 8192)	// 8k line limit prevents stack overflow in handling code 
	{
		outputLine.message = message;
		message.clear();
		lines.push_back(outputLine);
	}
	return lines;
}

Lines NewlineFilter::FlushLinesFromTerminatedProcesses(PIDMap terminatedProcessesMap)
{
	Lines lines;
	//lines.push_back(Line(0.0, FILETIME(), 0, "<debug>", "<newline cache checked>", nullptr));

	for (auto i = terminatedProcessesMap.begin(); i != terminatedProcessesMap.end(); i++)
	{
		DWORD pid = i->first;
		if (m_lineBuffers.find(pid) != m_lineBuffers.end())
		{
			if (!m_lineBuffers[pid].empty())
			{
				// timestamp not filled, this will be done by the loopback source
				lines.push_back(Line(0.0, FILETIME(), pid, "<flush>", m_lineBuffers[pid], nullptr));
			}
			m_lineBuffers.erase(pid);
		}
		lines.push_back(Line(0.0, FILETIME(), pid, "<debug>", "<terminated>", nullptr));
	}
	return lines;
}

} // namespace debugviewpp 
} // namespace fusion
