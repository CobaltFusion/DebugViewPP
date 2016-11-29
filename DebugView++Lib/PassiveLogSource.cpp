// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Win32/Win32Lib.h"
#include "DebugView++Lib/PassiveLogSource.h"
#include "DebugView++Lib/LineBuffer.h"

namespace fusion {
namespace debugviewpp {

PollLine::PollLine(DWORD pid, const std::string& processName, const std::string& message, const LogSource* pLogSource) :
	timesValid(false),
	time(0.0),
	systemTime(FILETIME()),
	pid(pid),
	processName(processName),
	message(message),
	pLogSource(pLogSource)
{
}
PollLine::PollLine(double time, FILETIME systemTime, DWORD pid, const std::string& processName, const std::string& message, const LogSource* pLogSource) :
	timesValid(true),
	time(time),
	systemTime(systemTime),
	pid(pid),
	processName(processName),
	message(message),
	pLogSource(pLogSource)
{
}

PassiveLogSource::PassiveLogSource(Timer& timer, SourceType::type sourceType, ILineBuffer& linebuffer, long pollFrequency) :
	LogSource(timer, sourceType, linebuffer),
	m_microsecondInterval(pollFrequency > 0 ? 1000000 / pollFrequency : 0),
	m_handle(Win32::CreateEvent(nullptr, false, false, nullptr))
{
}

void PassiveLogSource::StartThread()
{
	if (m_microsecondInterval > m_microsecondInterval.zero())
		m_thread = boost::thread(&PassiveLogSource::Loop, this);
}

long PassiveLogSource::GetMicrosecondInterval() const
{
	return static_cast<long>(m_microsecondInterval.count());
}

void PassiveLogSource::Abort()
{
	LogSource::Abort();
	m_thread.join();
}

void PassiveLogSource::Loop()
{
	for (;;)
	{
		Poll();
		Signal();
		if (LogSource::AtEnd())
			break;
		// sub 16ms sleep, depends on available hardware for accuracy
		boost::this_thread::sleep_for(m_microsecondInterval);
	}
}

HANDLE PassiveLogSource::GetHandle() const 
{
	return m_handle.get();
}

void PassiveLogSource::Notify()
{
	// this swap is essential for efficiency.
	{
		boost::mutex::scoped_lock lock(m_mutex);
		m_lines.swap(m_backBuffer);
	}

	for (auto& line : m_backBuffer)
	{
		if (line.timesValid)
			Add(line.time, line.systemTime, line.pid, line.processName, line.message);
		else
			Add(line.pid, line.processName, line.message);
	}
	m_backBuffer.clear();
}

void PassiveLogSource::Poll()
{
}

void PassiveLogSource::AddMessage(DWORD pid, const std::string& processName, const std::string& message)
{
	boost::mutex::scoped_lock lock(m_mutex);
	m_lines.push_back(PollLine(pid, processName, message, this));
}

void PassiveLogSource::AddMessage(const std::string& message)
{
	boost::mutex::scoped_lock lock(m_mutex);
	std::string msg = message + "\n";
	m_lines.push_back(PollLine(0, "[internal]", msg, this));
}

void PassiveLogSource::AddMessage(double time, FILETIME systemTime, DWORD pid, const std::string& processName, const std::string& message)
{
	m_lines.push_back(PollLine(time, systemTime, pid, processName, message, this));
}

void PassiveLogSource::Signal()
{
	boost::mutex::scoped_lock lock(m_mutex);
	if (!m_lines.empty())
		SetEvent(m_handle.get());
}

} // namespace debugviewpp 
} // namespace fusion
