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

LogSources::LogSources(bool startListening) : 
	m_end(false),
	m_sourcesDirty(false),
	m_updateEvent(CreateEvent(NULL, FALSE, FALSE, NULL)),
	m_waitHandles(GetWaitHandles())
{
	if (startListening)
	{
		m_thread = boost::thread(&LogSources::Listen, this);
	}
}
	
LogSources::~LogSources()
{
	Abort();
}

void LogSources::Add(std::shared_ptr<LogSource> source)
{
	boost::mutex::scoped_lock lock(m_mutex);
	m_sources.push_back(source);
	m_sourcesDirty = true;
	SetEvent(m_updateEvent.get());
	printf("add...");
}

void LogSources::Remove(std::shared_ptr<LogSource> logsource)
{
	boost::mutex::scoped_lock lock(m_mutex);
	//m_sources.erase(logsource
	//printf("remove...");
	//SetEvent(m_updateEvent.get());

	for (auto i = m_sources.begin(); i != m_sources.end(); i++)
	{
		if (i->get() == logsource.get())
		{
			printf("remove...");
			m_sources.erase(i);
			SetEvent(m_updateEvent.get());
			break;
		}
	}
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
		auto handle = (*i)->GetHandle();
		if (handle)
			handles.push_back(handle);
	}
	handles.push_back(m_updateEvent.get());

	return handles;
}

void LogSources::Listen()
{
	for (;;)
	{
		m_waitHandles = GetWaitHandles();
		for (;;)
		{
			auto res = WaitForAnyObject(m_waitHandles, INFINITE);
			if (m_end)
				break;
			if (res.signaled)
				if (res.index == (m_waitHandles.size()-1))
					break;
				else
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
