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
	m_waitHandles(GetWaitHandles()),
	m_thread(&LogSources::Run, this)
{
}
	
LogSources::~LogSources()
{
	Abort();
}


void LogSources::Add(std::unique_ptr<LogSource> source)
{
	boost::mutex::scoped_lock lock(m_mutex);
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

LogSourcesHandles LogSources::GetWaitHandles()
{
	boost::mutex::scoped_lock lock(m_mutex);
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
	return;

	for (;;)
	{
		for (;;)
		{
			auto res = WaitForAnyObject(m_waitHandles, 1000);
			if (m_end || m_sourcesDirty)
				break;
			if (res.signaled)
				Process(res.index);
		}
		if (m_end)
			break;
		m_waitHandles = GetWaitHandles();
	}
}

void LogSources::Process(int index)
{
	auto& logsource = m_sources[index];
	logsource->Notify();
	if (logsource->AtEnd())
		m_sources.erase(m_sources.begin() + index);
}

Lines LogSources::GetLines()
{
	if (m_sources.empty())
		return Lines();

	auto pred = [](const Line& a, const Line& b) { return a.time < b.time; };
	Lines lines;
	for (auto it = m_sources.begin(); it != m_sources.end(); )
	{
		Lines pipeLines((*it)->GetLines());
		Lines lines2;
		lines2.reserve(lines.size() + pipeLines.size());
		std::merge(lines.begin(), lines.end(), pipeLines.begin(), pipeLines.end(), std::back_inserter(lines2), pred);
		lines.swap(lines2);

		if ((*it)->AtEnd())
			it = m_sources.erase(it);	// todo: erase an element of the list we're iterating? 
		else
			++it;
	}
	return lines;
}

} // namespace debugviewpp 
} // namespace fusion
