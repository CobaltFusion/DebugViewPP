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

std::vector<std::shared_ptr<LogSource>> LogSources::GetSources()
{
	boost::mutex::scoped_lock lock(m_mutex);
	return m_sources;
}

LogSources::LogSources(bool startListening) : 
	m_end(false),
	m_autoNewLine(false),
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

void LogSources::SetAutoNewLine(bool value)
{
	m_autoNewLine = value;
	for (auto i = m_sources.begin(); i != m_sources.end(); i++)
	{
		auto source = *i;
		source->SetAutoNewLine(value);
	}
}

bool LogSources::GetAutoNewLine() const
{
	return m_autoNewLine;
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

InputLines LogSources::GetLines()
{
	auto inputLines = m_newlineFilter.Process(m_linebuffer.GetLines());

	InputLines lines;
	for (auto it = inputLines.begin(); it != inputLines.end(); ++it )
	{
		auto inputLine = *it;
		inputLine.pid = GetProcessId(inputLine.handle);
		if (inputLine.logsource != nullptr)
		{
			inputLine.processName = Str(inputLine.logsource->GetProcessName(inputLine.handle)).str();
		}
		
		if (inputLine.handle != INVALID_HANDLE_VALUE)
		{
			Handle processHandle(inputLine.handle);
			m_handleCache.Add(inputLine.pid, std::move(processHandle));
			inputLine.handle = 0;
		}
		lines.push_back(inputLine);
	}
	return lines;
}

std::shared_ptr<DBWinReader> LogSources::AddDBWinReader(bool global)
{
	auto dbwinreader = std::make_shared<DBWinReader>(m_linebuffer, global);
	dbwinreader->SetAutoNewLine(m_autoNewLine);
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
	processReader->SetAutoNewLine(m_autoNewLine);
	Add(processReader);
	return processReader;
}

std::shared_ptr<FileReader> LogSources::AddFileReader(const std::wstring& filename)
{
	auto filereader = std::make_shared<FileReader>(m_linebuffer, filename);
	filereader->SetAutoNewLine(m_autoNewLine);
	Add(filereader);
	return filereader;
}

std::shared_ptr<DBLogReader> LogSources::AddDBLogReader(const std::wstring& filename)
{
	auto dblogreader = std::make_shared<DBLogReader>(m_linebuffer, filename);
	dblogreader->SetAutoNewLine(m_autoNewLine);
	Add(dblogreader);
	return dblogreader;
}

std::shared_ptr<PipeReader> LogSources::AddPipeReader(DWORD pid, HANDLE hPipe)
{
	auto processName = Str(ProcessInfo::GetProcessNameByPid(pid)).str();
	auto pipeReader = std::make_shared<PipeReader>(m_linebuffer, hPipe, pid, processName);
	pipeReader->SetAutoNewLine(m_autoNewLine);
	return pipeReader;
}

} // namespace debugviewpp 
} // namespace fusion
