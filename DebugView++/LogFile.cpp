// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <vector>
#include "LogFile.h"
#include "Win32Lib.h"

namespace fusion {

bool LogFile::Empty() const
{
	return m_messages.empty();
}

void LogFile::Clear()
{
	m_messages.clear();
}

Message LogFile::Add(const Message& msg)
{
	Message message(msg);
	if (msg.handleValid)
	{
		Handle processHandle(msg.handle);
		auto props = m_processInfo.GetProcessProperties(msg.processId, processHandle.get());
		m_messages.push_back(InternalMessage(msg.time, msg.systemTime, props.uid, msg.text));
		message.processName = props.name;
	}
	else
	{
		auto props = m_processInfo.GetProcessProperties(msg.processId, msg.processName);
		m_messages.push_back(InternalMessage(msg.time, msg.systemTime, props.uid, msg.text));
		message.processName = props.name;
	}
	return message;
}

int LogFile::Count() const
{
	return m_messages.size();
}

Message LogFile::operator[](int i) const
{
	auto msg = m_messages[i];
	auto props = m_processInfo.GetProcessProperties(msg.uid);
	return Message(msg.time, msg.systemTime, props.pid, props.name, msg.text);
}

} // namespace fusion
