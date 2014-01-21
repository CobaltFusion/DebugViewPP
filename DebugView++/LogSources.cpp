// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "stdafx.h"
#include "LogSources.h"

namespace fusion {
namespace debugviewpp {

LogSourceInfo::LogSourceInfo(HANDLE handle, LogSource& logsource) :
	handle(handle), 
	logsource(logsource)
{

}

LogSources::LogSources() : 
	m_end(false),
	m_sourcesDirty(false),
	m_updateEvent(CreateEvent(NULL, FALSE, FALSE, NULL)),
	m_thread(&LogSources::Run, this)
{

}

void LogSources::Add(std::unique_ptr<LogSource> source)
{
	m_sources.push_back(std::move(source));
	m_sourcesDirty = true;
	SetEvent(m_updateEvent.get());
}

void LogSources::Abort()
{
	m_end = true;
	SetEvent(m_updateEvent.get());
	m_thread.join();
}

LogSourcesHandles LogSources::GetWaitHandles() const
{
	LogSourcesHandles handles;
	for (auto i = m_sources.begin(); i != m_sources.end(); i++)
	{
		handles.push_back(i->get());
	}
	handles.push_back(m_updateEvent.get());
	return handles;
}

void LogSources::Run()
{
	for (;;)
	{
		auto handles = GetWaitHandles();
		for (;;)
		{
			auto res = WaitForAnyObject(handles, 1000);
			if (m_end || m_sourcesDirty)
				break;
			if (res.signaled)
				Process(res.index);
		}
		if (m_end)
			break;
	}
}

void LogSources::Process(int index)
{
	auto& logsource = m_sources[index];
	logsource->Notify();
	if (logsource->AtEnd())
		m_sources.erase(m_sources.begin() + index);
}


LogSources::~LogSources()
{
}

} // namespace debugviewpp 
} // namespace fusion
