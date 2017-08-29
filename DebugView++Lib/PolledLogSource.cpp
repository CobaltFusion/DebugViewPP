// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Win32/Win32Lib.h"
#include "DebugView++Lib/PolledLogSource.h"
#include "DebugView++Lib/LineBuffer.h"
#include <iostream>

namespace fusion {
namespace debugviewpp {


PollLine::PollLine(Win32::Handle handle, const std::string& message, const LogSource* pLogSource) :
	timesValid(false),
	time(0.0),
	systemTime(FILETIME()),
	handle(std::move(handle)),
	pid(0),
	message(message),
	pLogSource(pLogSource)
{
}

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

PolledLogSource::PolledLogSource(Timer& timer, SourceType::type sourceType, ILineBuffer& linebuffer, long pollFrequency) :
	LogSource(timer, sourceType, linebuffer),
	m_microsecondInterval(pollFrequency > 0 ? 1000000 / pollFrequency : 0),
	m_handle(Win32::CreateEvent(nullptr, false, false, nullptr))
{
}

PolledLogSource::~PolledLogSource()
{
	Abort();
}

void PolledLogSource::StartThread()
{
	if (m_microsecondInterval > m_microsecondInterval.zero())
		m_thread = std::make_unique<std::thread>(&PolledLogSource::Loop, this);
}

long PolledLogSource::GetMicrosecondInterval() const
{
	return static_cast<long>(m_microsecondInterval.count());
}

void PolledLogSource::Abort()
{
	LogSource::Abort();
	if (m_thread) m_thread->join();
	m_thread.reset();
	Notify();
}

void PolledLogSource::Loop()
{
	for (;;)
	{
		Poll();
		Signal();
		if (LogSource::AtEnd())
			break;
		// sub 16ms sleep, depends on available hardware for accuracy
		std::this_thread::sleep_for(m_microsecondInterval);
	}
}

HANDLE PolledLogSource::GetHandle() const 
{
	return m_handle.get();
}

void PolledLogSource::Notify()
{
	// this swap is essential for efficiency.
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_lines.swap(m_backBuffer);
	}

	for (auto& line : m_backBuffer)
	{
		if (line.handle)
			Add(line.handle.release(), line.message);
		else if (line.timesValid)
			Add(line.time, line.systemTime, line.pid, line.processName, line.message);
		else
			Add(line.pid, line.processName, line.message);
	}
	m_backBuffer.clear();
}

void PolledLogSource::Poll()
{
}

void PolledLogSource::AddMessage(Win32::Handle handle, const std::string& message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lines.push_back(PollLine(std::move(handle), message, this));
}

void PolledLogSource::AddMessage(DWORD pid, const std::string& processName, const std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_lines.push_back(PollLine(pid, processName, message, this));
}

void PolledLogSource::AddMessage(const std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	std::string msg = message;
	m_lines.push_back(PollLine(0, "[internal]", msg, this));
}

void PolledLogSource::AddMessage(double time, FILETIME systemTime, DWORD pid, const std::string& processName, const std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_lines.push_back(PollLine(time, systemTime, pid, processName, message, this));
}

void PolledLogSource::Signal()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	if (!m_lines.empty())
		SetEvent(m_handle.get());
}

} // namespace debugviewpp 
} // namespace fusion
