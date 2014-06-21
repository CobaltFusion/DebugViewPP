// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DebugView++Lib/PolledLogSource.h"
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

PolledLogSource::PolledLogSource(ILineBuffer& linebuffer, long pollFrequency) :
	LogSource(SourceType::System, linebuffer),
	m_handle(CreateEvent(NULL, TRUE, FALSE, L"PolledLogSourceEvent"))
{
	if (pollFrequency > 0)
	{
		m_microsecondInterval = 1000000/pollFrequency;
		m_thread = boost::thread(&PolledLogSource::Loop, this);
	}
}

PolledLogSource::~PolledLogSource()
{
	Abort();
}

void PolledLogSource::Abort()
{
	LogSource::Abort();
	m_thread.join();
}

void PolledLogSource::Loop()
{
	for(;;)
	{
		if (Poll())
		{
			Signal();
		}
		if (AtEnd())
			break;
		// sub 16ms sleep, depends on available hardware for accuracy
		boost::this_thread::sleep(boost::posix_time::microseconds(m_microsecondInterval));
	}
}

HANDLE PolledLogSource::GetHandle() const 
{
	return m_handle.get();
}

void PolledLogSource::Notify()
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

void PolledLogSource::AddMessage(DWORD pid, const char* processName, const char* message)
{
	boost::mutex::scoped_lock lock(m_mutex);
	m_lines.push_back(LoopLine(pid, processName, message, this));
}

void PolledLogSource::Signal()
{
	boost::mutex::scoped_lock lock(m_mutex);
	if (!m_lines.empty())
	{
		SetEvent(m_handle.get());
	}
}

} // namespace debugviewpp 
} // namespace fusion
