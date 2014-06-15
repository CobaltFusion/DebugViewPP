// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DebugView++Lib/Loopback.h"
#include "DebugView++Lib/LineBuffer.h"

namespace fusion {
namespace debugviewpp {

LoopLine::LoopLine(DWORD pid, const std::string& processName, const std::string& message, LogSource* logsource) :
	pid(pid),
	processName(processName),
	message(message),
	logsource(logsource)
{
}

Loopback::Loopback(ILineBuffer& linebuffer) :
	LogSource(SourceType::System, linebuffer),
	m_handle(CreateEvent(NULL, TRUE, FALSE, L"LoopbackEvent"))
{
	SetDescription(L"Loopback");
}

Loopback::~Loopback()
{
}

bool Loopback::GetAutoNewLine() const
{
	return true;
}

bool Loopback::AtEnd() const
{
	return false;
}

HANDLE Loopback::GetHandle() const 
{
	return m_handle.get();
}

void Loopback::PreProcess(Line& line) const
{
}

void Loopback::Notify()
{
	boost::mutex::scoped_lock lock(m_mutex);
	for (auto i = m_lines.cbegin(); i != m_lines.cend(); ++i)
	{
		auto line = *i;
		Add(line.pid, line.processName.c_str(), line.message.c_str(), this);
	}
	m_lines.clear();
	ResetEvent(m_handle.get());
}

void Loopback::AddMessage(DWORD pid, const char* processName, const char* message)
{
	boost::mutex::scoped_lock lock(m_mutex);
	m_lines.push_back(LoopLine(pid, processName, message, this));
}

void Loopback::Signal()
{
	boost::mutex::scoped_lock lock(m_mutex);
	if (!m_lines.empty())
	{
		SetEvent(m_handle.get());
	}
}


} // namespace debugviewpp 
} // namespace fusion
