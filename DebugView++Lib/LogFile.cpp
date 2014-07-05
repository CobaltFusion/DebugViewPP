// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <vector>
#include "DebugView++Lib/LogFile.h"

namespace fusion {
namespace debugviewpp {

Message::Message(double time, FILETIME systemTime, DWORD pid, const std::string processName, const std::string& msg, COLORREF color) :
	time(time), systemTime(systemTime), processId(pid), processName(processName), text(msg), color(color)
{
}

bool LogFile::Empty() const
{
	return m_messages.empty();
}

void LogFile::Clear()
{
	m_messages.clear();
	m_messages.shrink_to_fit();
	m_storage.Clear();
	m_processInfo.Clear();
}

void LogFile::Add(const Message& msg)
{
	auto props = m_processInfo.GetProcessProperties(msg.processId, msg.processName);
	m_messages.push_back(InternalMessage(msg.time, msg.systemTime, props.uid));
	m_storage.Add(msg.text);
}

int LogFile::Count() const
{
	return m_messages.size();
}

Message LogFile::operator[](int i) const
{
	auto msg = m_messages[i];
	auto props = m_processInfo.GetProcessProperties(msg.uid);
	return Message(msg.time, msg.systemTime, props.pid, props.name, m_storage[i], props.color);
}

} // namespace debugviewpp 
} // namespace fusion
