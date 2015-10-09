// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Win32Lib/Win32Lib.h"
#include "DebugView++Lib/PassiveLogSource.h"
#include "DebugView++Lib/LineBuffer.h"

namespace fusion {
namespace debugviewpp {

PollLine::PollLine(DWORD pid, const std::string& processName, const std::string& message, const std::shared_ptr<LogSource>& pLogSource) :
	pid(pid),
	processName(processName),
	message(message),
	pLogSource(pLogSource)
{
}

PassiveLogSource::PassiveLogSource(Timer& timer, SourceType::type sourceType, ILineBuffer& linebuffer, long pollFrequency) :
	LogSource(timer, sourceType, linebuffer),
	m_microsecondInterval(0),
	m_handle(CreateEvent(nullptr, false, false, nullptr))
{
	if (pollFrequency > 0)
	{
		m_microsecondInterval = 1000000/pollFrequency;
	}
}

PassiveLogSource::~PassiveLogSource()
{
}

void PassiveLogSource::StartThread()
{
	if (m_microsecondInterval > 0)
	{
		m_thread = boost::thread(&PassiveLogSource::Loop, this);
	}
}

long PassiveLogSource::GetMicrosecondInterval() const
{
	return m_microsecondInterval;
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
		boost::this_thread::sleep(boost::posix_time::microseconds(m_microsecondInterval));
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

	for (auto it = m_backBuffer.cbegin(); it != m_backBuffer.cend(); ++it)
		Add(it->pid, it->processName, it->message);

	m_backBuffer.clear();
}

void PassiveLogSource::AddMessage(DWORD pid, const std::string& processName, const std::string& message)
{
	boost::mutex::scoped_lock lock(m_mutex);
	m_lines.push_back(PollLine(pid, processName, message, shared_from_this()));
}

void PassiveLogSource::AddMessage(const std::string& message)
{
	boost::mutex::scoped_lock lock(m_mutex);
	m_lines.push_back(PollLine(0, "[internal]", message, shared_from_this()));
}

void PassiveLogSource::Signal()
{
	boost::mutex::scoped_lock lock(m_mutex);
	if (!m_lines.empty())
		SetEvent(m_handle.get());
}

} // namespace debugviewpp 
} // namespace fusion
