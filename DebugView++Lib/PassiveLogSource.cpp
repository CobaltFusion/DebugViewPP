// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DebugView++Lib/PassiveLogSource.h"
#include "DebugView++Lib/LineBuffer.h"

namespace fusion {
namespace debugviewpp {

PollLine::PollLine(DWORD pid, const std::string& processName, const std::string& message, std::shared_ptr<LogSource> logsource) :
	pid(pid),
	processName(processName),
	message(message),
	logsource(logsource)
{
}

PassiveLogSource::PassiveLogSource(Timer& timer, SourceType::type sourceType, ILineBuffer& linebuffer, long pollFrequency) :
	LogSource(timer, sourceType, linebuffer),
	m_microsecondInterval(0),
	m_handle(CreateEvent(NULL, FALSE, FALSE, NULL))
{
	if (pollFrequency > 0)
	{
		m_microsecondInterval = 1000000/pollFrequency;
	}
}

PassiveLogSource::~PassiveLogSource()
{
    std::cout << " # ~PassiveLogSource()" << std::endl;
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
	for(;;)
	{
		Poll();
		Signal();
		if (AtEnd())
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
	//m_backLines.clear(); //todo:: swap

	boost::mutex::scoped_lock lock(m_mutex);
	for (auto i = m_lines.cbegin(); i != m_lines.cend(); ++i)
	{
		auto line = *i;
		Add(line.pid, line.processName.c_str(), line.message.c_str());
	}
	m_lines.clear();
}

void PassiveLogSource::AddMessage(DWORD pid, const char* processName, const char* message)
{
	boost::mutex::scoped_lock lock(m_mutex);
	m_lines.push_back(PollLine(pid, processName, message, shared_from_this()));
}

void PassiveLogSource::AddMessage(const std::string& message)
{
	boost::mutex::scoped_lock lock(m_mutex);
	m_lines.push_back(PollLine(0, "[internal]", message.c_str(), shared_from_this()));
}

void PassiveLogSource::Signal()
{
	boost::mutex::scoped_lock lock(m_mutex);
	if (!m_lines.empty())
		SetEvent(m_handle.get());
}

} // namespace debugviewpp 
} // namespace fusion
