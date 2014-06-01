// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "stdafx.h"
#include "DebugView++Lib/LogSources.h"
#include "DebugView++Lib/ProcessReader.h"
#include "DebugView++Lib/PipeReader.h"
#include "DebugView++Lib/FileReader.h"
#include "DebugView++Lib/DBWinReader.h"
#include "DebugView++Lib/TestSource.h"
#include "DebugView++Lib/ProcessInfo.h"
#include "Win32Lib/utilities.h"
#include "DebugView++Lib/LineBuffer.h"
#include "DebugView++Lib/VectorLineBuffer.h"

// class Logsources heeft vector<LogSource> en start in zijn constructor een thread voor LogSources::Listen()
// - Listen() vraagt GetHandle() aan elke LogSource in m_sources en roept Notify() op de LogSource waarvan de handle gesignaled wordt.
// - the LogSource::Notify leest input en schrijft deze naar de linebuffer die hij bij constructie gekregen heeft.
// 

namespace fusion {
namespace debugviewpp {

LogSourceInfo::LogSourceInfo(HANDLE handle, LogSource& logsource) :
	handle(handle), 
	logsource(logsource)
{

}

std::vector<std::shared_ptr<LogSource>> LogSources::GetSources()
{
	boost::mutex::scoped_lock lock(m_mutex);
	return m_sources;
}

LogSources::LogSources(bool startListening) : 
	m_end(false),
	m_updateEvent(CreateEvent(NULL, FALSE, FALSE, NULL)),
	m_linebuffer(64*1024)
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
	SetEvent(m_updateEvent.get());
}

void LogSources::Remove(std::shared_ptr<LogSource> logsource)
{
	boost::mutex::scoped_lock lock(m_mutex);
	m_sources.erase(std::remove(m_sources.begin(), m_sources.end(), logsource), m_sources.end());
}

void LogSources::Abort()
{
	m_end = true;
	SetEvent(m_updateEvent.get());
	m_thread.join();
}

LogSourcesHandles LogSources::GetWaitHandles(std::vector<std::shared_ptr<LogSource>>& logsources) const
{
	LogSourcesHandles handles;
	for (auto i = logsources.begin(); i != logsources.end(); i++)
	{
		auto handle = (*i)->GetHandle();
		if (handle)
			handles.push_back(handle);
	}
	return handles;
}

void LogSources::Listen()
{
	for (;;)
	{
		auto sources = GetSources();
		auto waitHandles = GetWaitHandles(sources);
		auto updateEventIndex = waitHandles.size(); 
		waitHandles.push_back(m_updateEvent.get());

		for (;;)
		{
			auto res = WaitForAnyObject(waitHandles, INFINITE);
			if (m_end)
				break;
			if (res.signaled)
				if (res.index == updateEventIndex)
					break;
				else
				{
					auto logsource = sources[res.index];
					Process(logsource);
				}
		}
		if (m_end)
			break;
	}
}

void LogSources::Process(std::shared_ptr<LogSource> logsource)
{
	logsource->Notify();
	if (logsource->AtEnd())
	{
		Remove(logsource);
	}
}

#ifdef ENABLE_EXPERIMENTAL_CIRCULAR_BUFFER
Lines LogSources::GetLines()
{
	return m_linebuffer.GetLines();
}
#else
Lines LogSources::GetLines()			// depricated, remove
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
#endif
std::shared_ptr<DBWinReader> LogSources::AddDBWinReader(bool global)
{
	auto dbwinreader = std::make_shared<DBWinReader>(m_linebuffer, global);
	Add(dbwinreader);
	return dbwinreader;
}

std::shared_ptr<TestSource> LogSources::AddTestSource()
{
	auto testsource = std::make_shared<TestSource>(m_linebuffer);
	Add(testsource);
	return testsource;
}


std::shared_ptr<ProcessReader> LogSources::AddProcessReader(const std::wstring& pathName, const std::wstring& args)
{
	auto processReader = std::make_shared<ProcessReader>(m_linebuffer, pathName, args);
	Add(processReader);
	return processReader;
}

std::shared_ptr<FileReader> LogSources::AddFileReader(const std::wstring& filename)
{
	auto filereader = std::make_shared<FileReader>(m_linebuffer, filename);
	Add(filereader);
	return filereader;
}

std::shared_ptr<DBLogReader> LogSources::AddDBLogReader(const std::wstring& filename)
{
	auto dblogreader = std::make_shared<DBLogReader>(m_linebuffer, filename);
	Add(dblogreader);
	return dblogreader;
}

std::shared_ptr<PipeReader> LogSources::AddPipeReader(DWORD pid, HANDLE hPipe)
{
	auto processName = Str(ProcessInfo::GetProcessNameByPid(pid)).str();
	auto pipeReader = std::make_shared<PipeReader>(m_linebuffer, hPipe, pid, processName);
	return pipeReader;
}

} // namespace debugviewpp 
} // namespace fusion
