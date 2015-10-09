// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Win32Lib/Win32Lib.h"
#include "DebugView++Lib/NewlineFilter.h"
#include "DebugView++Lib/LogSource.h"
#include "DebugView++Lib/ProcessInfo.h"

namespace fusion {
namespace debugviewpp {

NewlineFilter::NewlineFilter()
{
}

Lines NewlineFilter::Process(const Line& line)
{
	Lines lines;
	auto& message = m_lineBuffers[line.pid];
	message.reserve(4000);

	Line outputLine = line;
	for (auto it = line.message.begin(); it != line.message.end(); ++it)
	{
		if (*it == '\r')
			continue;

		if (*it == '\n')
		{
			outputLine.message = message;
			message.clear();
			lines.push_back(outputLine);
		}
		else
		{
			message.push_back(*it);
		}
	}

	if (message.empty())
	{
		m_lineBuffers.erase(line.pid);
	}
	else if (outputLine.pLogSource->GetAutoNewLine() || message.size() > 8192)	// 8k line limit prevents stack overflow in handling code 
	{
		outputLine.message = message;
		message.clear();
		lines.push_back(outputLine);
	}
	return lines;
}

Lines NewlineFilter::FlushLinesFromTerminatedProcesses(const PidMap& terminatedProcessesMap)
{
	Lines lines;
	for (auto it = terminatedProcessesMap.begin(); it != terminatedProcessesMap.end(); ++it)
	{
		DWORD pid = it->first;
		HANDLE handle = it->second.get();
		if (m_lineBuffers.find(pid) != m_lineBuffers.end())
		{
			if (!m_lineBuffers[pid].empty())
			{
				// timestamp not filled, this will be done by the loopback source
				lines.push_back(Line(0, FILETIME(), pid, "<flush>", m_lineBuffers[pid], nullptr));
			}
			m_lineBuffers.erase(pid);
		}
		auto processName = Str(ProcessInfo::GetProcessName(handle)).str();
		auto info = ProcessInfo::GetProcessInfo(handle);
		std::string infoStr = stringbuilder() << "<process started at " << info << " has now terminated>";
		lines.push_back(Line(0, FILETIME(), pid, processName, infoStr, nullptr));
	}
	return lines;
}

} // namespace debugviewpp 
} // namespace fusion
