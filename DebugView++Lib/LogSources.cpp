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
#include "DebugView++Lib/DbgviewReader.h"
#include "DebugView++Lib/SocketReader.h"
#include "DebugView++Lib/TestSource.h"
#include "DebugView++Lib/ProcessInfo.h"
#include "Win32Lib/utilities.h"
#include "DebugView++Lib/LineBuffer.h"
#include "DebugView++Lib/VectorLineBuffer.h"
#include "DebugView++Lib/Loopback.h"
#include <boost/algorithm/string.hpp>


// class Logsources has a vector<LogSource> and start a thread for LogSources::Listen()
// - Listen() exectues every LogSource::GetHandle() in m_sources and calls Notify() for any signaled handle.
// - LogSource::Notify reads input en writes to linebuffer (passed at construction)
// 

namespace fusion {
namespace debugviewpp {

const double g_handleCacheTimeout = 5.0; //seconds

LogSources::LogSources(bool startListening) : 
	m_end(false),
	m_autoNewLine(true),
	m_updateEvent(CreateEvent(NULL, FALSE, FALSE, NULL)),
	m_linebuffer(64*1024),
	m_loopback(std::make_shared<Loopback>(m_timer, m_linebuffer)),
	m_handleCacheTime(0.0)
{
	m_sources.push_back(m_loopback);
	if (startListening)
		m_thread = boost::thread(&LogSources::Listen, this);
}
	
LogSources::~LogSources()
{
	Abort();
}

void LogSources::AddMessage(const std::string& message)
{
	m_loopback->AddMessage(message);
	m_loopback->Signal();
}

void LogSources::Add(std::shared_ptr<LogSource> source)
{
	boost::mutex::scoped_lock lock(m_mutex);
	m_sources.push_back(source);
	SetEvent(m_updateEvent.get());
}

void LogSources::Remove(std::shared_ptr<LogSource> logsource)
{
	InternalRemove(logsource);
	SetEvent(m_updateEvent.get());
}

void LogSources::InternalRemove(std::shared_ptr<LogSource> logsource)
{
	AddMessage(stringbuilder() << "Source '" << logsource->GetDescription() << "' was removed.");
	boost::mutex::scoped_lock lock(m_mutex);
	logsource->Abort();

	m_sources.erase(std::remove(m_sources.begin(), m_sources.end(), logsource), m_sources.end());
}

std::vector<std::shared_ptr<LogSource>> LogSources::GetSources()
{
	std::vector<std::shared_ptr<LogSource>> sources;
	boost::mutex::scoped_lock lock(m_mutex);
	for (auto i = m_sources.begin(); i != m_sources.end(); ++i)
	{
		if ((*i) == m_loopback)
			continue;				
		sources.push_back(*i);
	}
	return sources;
}

void LogSources::SetAutoNewLine(bool value)
{
	m_autoNewLine = value;
	for (auto i = m_sources.begin(); i != m_sources.end(); ++i)
	{
		(*i)->SetAutoNewLine(value);
	}
}

bool LogSources::GetAutoNewLine() const
{
	return m_autoNewLine;
}

void LogSources::Abort()
{
	m_end = true;

	boost::unique_lock<boost::mutex> lock(m_mutex);
	auto sources = m_sources;
	lock.unlock();
	for (auto i = sources.begin(); i != sources.end(); ++i)
	{
		(*i)->Abort();
	}
	SetEvent(m_updateEvent.get());
	m_thread.join();
}

void LogSources::Reset()
{
	m_timer.Reset();
}

// default behaviour: 
// LogSources starts with 1 logsource, the loopback source
// At startup normally 1 DBWinReader is added by m_logSources.AddDBWinReader
void LogSources::Listen()
{
	for (;;)
	{
		std::vector<HANDLE> waitHandles;
		std::vector<std::shared_ptr<LogSource>> sources;
		{
			boost::mutex::scoped_lock lock(m_mutex);
			for (auto it = m_sources.begin(); it != m_sources.end(); ++it)
			{
				auto source = *it;
				HANDLE handle = source->GetHandle();
				if (handle != INVALID_HANDLE_VALUE)
				{
					waitHandles.push_back(handle);
					sources.push_back(source);
				}
				source->Initialize();	//todo: find a better way to do this?
			}
		}

		auto updateEventIndex = waitHandles.size(); 
		waitHandles.push_back(m_updateEvent.get());
		for (;;)
		{
			m_loopback->Signal();
			auto res = WaitForAnyObject(waitHandles, INFINITE);

			if (m_end)
				break;
			if (res.signaled)
			{
				int index = res.index - WAIT_OBJECT_0;
				if (index == updateEventIndex)
					break;
				else
				{
					if (index >= (int)sources.size())
					{
						assert(!"res.index out of range");
					}
					auto logsource = sources[index];
					logsource->Notify();
					if (logsource->AtEnd())
					{
						InternalRemove(logsource);
						break;
					}
				}
			}
		}
		if (m_end)
			break;
	}
}

void LogSources::CheckForTerminatedProcesses()
{
	if ((m_timer.Get() - m_handleCacheTime) < g_handleCacheTimeout)
		return;
	
	// add messages to inputLines would mess up the timestamp-order
	// instead put them in the m_loopback buffer for processing.

	auto flushedLines = m_newlineFilter.FlushLinesFromTerminatedProcesses(m_handleCache.CleanupMap());
	for (auto it = flushedLines.begin(); it != flushedLines.end(); ++it )
	{
		Line& line = *it;
		m_loopback->AddMessage(line.pid, line.processName.c_str(), line.message.c_str());
	}
	if (!flushedLines.empty())
		m_loopback->Signal();
	m_handleCacheTime = m_timer.Get();
}

Lines LogSources::GetLines()
{
	CheckForTerminatedProcesses();

	auto inputLines = m_linebuffer.GetLines();
	Lines lines;
	for (auto it = inputLines.begin(); it != inputLines.end(); ++it )
	{
		auto inputLine = *it;
		// let the logsource decide how to create processname
		if (inputLine.logsource != nullptr)
		{
			inputLine.logsource->PreProcess(inputLine);
		}
		
		if (inputLine.handle != 0)
		{
			inputLine.pid = GetProcessId(inputLine.handle);
			m_handleCache.Add(inputLine.pid, std::move(Handle(inputLine.handle)));
			inputLine.handle = 0;
		}

		// since a line can contain multiple newlines, processing 1 line can output
		// multiple lines, in this case the timestamp for each line is the same.
		auto processedLines = m_newlineFilter.Process(inputLine);
		for (auto it = processedLines.begin(); it != processedLines.end(); ++it )
		{
			auto& line = *it;
			const char* whitespace = " \r\n\t";
			boost::trim_right_if(line.message, boost::is_any_of(whitespace));
			lines.push_back(*it);
		}
	}
	return lines;
}

std::shared_ptr<DBWinReader> LogSources::AddDBWinReader(bool global)
{
	auto dbwinreader = std::make_shared<DBWinReader>(m_timer, m_linebuffer, global);
	Add(dbwinreader);
	return dbwinreader;
}

std::shared_ptr<TestSource> LogSources::AddTestSource()
{
	auto testsource = std::make_shared<TestSource>(m_timer, m_linebuffer);
	Add(testsource);
	return testsource;
}

std::shared_ptr<ProcessReader> LogSources::AddProcessReader(const std::wstring& pathName, const std::wstring& args)
{
	auto processReader = std::make_shared<ProcessReader>(m_timer, m_linebuffer, pathName, args);
	Add(processReader);
	return processReader;
}

std::shared_ptr<FileReader> LogSources::AddFileReader(const std::wstring& filename)
{
	auto filereader = std::make_shared<FileReader>(m_timer, m_linebuffer, IdentifyFile(Str(filename)), filename);
	Add(filereader);
	return filereader;
}

std::shared_ptr<DBLogReader> LogSources::AddDBLogReader(const std::wstring& filename)
{
	auto filetype = IdentifyFile(Str(filename));
	if (filetype == FileType::Unknown)
	{
		AddMessage(stringbuilder() << "Unable to open '" << filename <<"'\n");
		return std::shared_ptr<DBLogReader>();
	}
	AddMessage(stringbuilder() << "Started tailing " << filename << " identified as '" << FileTypeToString(filetype) << "'\n");

	auto dblogreader = std::make_shared<DBLogReader>(m_timer, m_linebuffer, filetype, filename);
	Add(dblogreader);
	return dblogreader;
}

std::shared_ptr<PipeReader> LogSources::AddPipeReader(DWORD pid, HANDLE hPipe)
{
	auto processName = Str(ProcessInfo::GetProcessNameByPid(pid)).str();
	auto pipeReader = std::make_shared<PipeReader>(m_timer, m_linebuffer, hPipe, pid, processName, 40);
	Add(pipeReader);
	return pipeReader;
}

std::shared_ptr<DbgviewReader> LogSources::AddDbgviewReader(const std::string& hostname)
{
	auto dbgviewreader = std::make_shared<DbgviewReader>(m_timer, m_linebuffer, hostname);
	m_loopback->AddMessage(stringbuilder() << "Source '" << dbgviewreader->GetDescription() << "' was added.");
	Add(dbgviewreader);
	return dbgviewreader;
}

std::shared_ptr<SocketReader> LogSources::AddUDPReader(const std::string& hostname, int port)
{
	auto socketreader = std::make_shared<SocketReader>(m_timer, m_linebuffer, hostname, port);
	m_loopback->AddMessage(stringbuilder() << "Source '" << socketreader->GetDescription() << "' was added.");
	Add(socketreader);
	return socketreader;
}

} // namespace debugviewpp 
} // namespace fusion
