// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <boost/algorithm/string.hpp>
#include "Win32Lib/Win32Lib.h"
#include "Win32Lib/utilities.h"
#include "DebugView++Lib/LogSources.h"
#include "DebugView++Lib/ProcessReader.h"
#include "DebugView++Lib/PipeReader.h"
#include "DebugView++Lib/FileReader.h"
#include "DebugView++Lib/BinaryFileReader.h"
#include "DebugView++Lib/DBLogReader.h"
#include "DebugView++Lib/DBWinReader.h"
#include "DebugView++Lib/DbgviewReader.h"
#include "DebugView++Lib/SocketReader.h"
#include "DebugView++Lib/TestSource.h"
#include "DebugView++Lib/ProcessInfo.h"
#include "DebugView++Lib/Conversions.h"
#include "DebugView++Lib/LineBuffer.h"
#include "DebugView++Lib/VectorLineBuffer.h"
#include "DebugView++Lib/Loopback.h"

// class Logsources has a vector<LogSource> and start a thread for LogSources::Listen()
// - Listen() exectues every LogSource::GetHandle() in m_sources and calls Notify() for any signaled handle.
// - LogSource::Notify reads input en writes to linebuffer (passed at construction)
// 

namespace fusion {
namespace debugviewpp {

const boost::chrono::seconds handleCacheTimeout(5);

LogSources::LogSources(bool startListening) : 
	m_end(false),
	m_autoNewLine(true),
	m_updateEvent(CreateEvent(nullptr, false, false, nullptr)),
	m_linebuffer(64*1024),
	m_loopback(std::make_shared<Loopback>(m_timer, m_linebuffer)),
	m_dirty(false)
{
	m_sources.push_back(m_loopback);
	if (startListening)
		m_listenThread = boost::thread(&LogSources::Listen, this);
	m_processMonitor.ConnectProcessEnd([this](DWORD pid, HANDLE handle) { OnProcessEnd(pid, handle); });
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

void LogSources::UpdateSettings(std::shared_ptr<LogSource> source)
{
	source->SetAutoNewLine(GetAutoNewLine());
}

void LogSources::Add(std::shared_ptr<LogSource> source)
{
	boost::mutex::scoped_lock lock(m_mutex);
	UpdateSettings(source);
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
	for (auto it = m_sources.begin(); it != m_sources.end(); ++it)
	{
		if (*it != m_loopback)
			sources.push_back(*it);
	}
	return sources;
}

void LogSources::SetAutoNewLine(bool value)
{
	m_autoNewLine = value;
	for (auto it = m_sources.begin(); it != m_sources.end(); ++it)
	{
		(*it)->SetAutoNewLine(value);
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
	for (auto it = sources.begin(); it != sources.end(); ++it)
	{
		(*it)->Abort();
	}
	SetEvent(m_updateEvent.get());
	m_listenThread.join();
}

void LogSources::Reset()
{
	m_timer.Reset();
}

boost::signals2::connection LogSources::SubscribeToUpdate(Update::slot_type slot)
{
	return m_update.connect(slot);
}

const boost::chrono::milliseconds graceTime(40); // -> intentionally near what the human eye can still perceive

void LogSources::OnUpdate()
{
	m_dirty = true;
	m_guiExecutor.CallAfter(graceTime, [this]() { DelayedUpdate(); });
}

void LogSources::DelayedUpdate()
{
	auto receivedLines = m_update();
	if (!receivedLines)
		return;

	if (receivedLines.get())	// get the actual return value
	{
		// messages where received, schedule next update
		m_guiExecutor.CallAfter(graceTime, [this]() { DelayedUpdate(); });
	}
	else
	{
		// no more messages where received
		m_dirty = false;
		// schedule one more update to workaround the race-condition writing to m_dirty
		// this avoids the need for locking in the extremly time-critical ListenUntilUpdateEvent() method
		m_guiExecutor.CallAfter(graceTime, [this]() { m_update(); });
	}
}

// default behaviour: 
// LogSources starts with 1 logsource, the loopback source
// At startup normally 1 DBWinReader is added by m_logSources.AddDBWinReader
void LogSources::Listen()
{
	while (!m_end)
		ListenUntilUpdateEvent();
}

void LogSources::ListenUntilUpdateEvent()
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
			// here LogSource::Initialize is called on the m_listenThread, currently only FileReader::Initialize uses this to start reading from the file.
			// This will block all other logsources while the file is being read (which is normally not a problem)
			source->Initialize();
		}
	}

	auto updateEventIndex = waitHandles.size(); 
	waitHandles.push_back(m_updateEvent.get());
	while (!m_end)
	{
		m_loopback->Signal();
		auto res = WaitForAnyObject(waitHandles, INFINITE);

		if (res.signaled)
		{
			int index = res.index - WAIT_OBJECT_0;
			if (index == updateEventIndex)
			{
				break;
			}
			else
			{
				assert((index < static_cast<int>(sources.size())) && "res.index out of range");
				auto logsource = sources[index];
				logsource->Notify();
				if (logsource->AtEnd())
				{
					InternalRemove(logsource);
					break;
				}
				if (!m_dirty)
					OnUpdate();
			}
		}
	}
}

void LogSources::OnProcessEnd(DWORD pid, HANDLE handle)
{
	m_guiExecutor.CallAsync([this, pid, handle]
	{
		auto flushedLines = m_newlineFilter.FlushLinesFromTerminatedProcess(pid, handle);
		for (auto it = flushedLines.begin(); it != flushedLines.end(); ++it)
			m_loopback->AddMessage(it->pid, it->processName, it->message);

		if (!flushedLines.empty())
			m_loopback->Signal();
		auto it = m_pidMap.find(pid);
		if (it != m_pidMap.end())
			m_pidMap.erase(it);
	});
}

Lines LogSources::GetLines()
{
	auto inputLines = m_linebuffer.GetLines();
	Lines lines;
	for (auto it = inputLines.begin(); it != inputLines.end(); ++it)
	{
		auto inputLine = *it;
		// let the logsource decide how to create processname
		if (inputLine.pLogSource)
		{
			inputLine.pLogSource->PreProcess(inputLine);
		}

		if (inputLine.handle)
		{
			Handle handle(inputLine.handle);
			inputLine.pid = GetProcessId(inputLine.handle);
			auto it = m_pidMap.find(inputLine.pid);
			if (it == m_pidMap.end())
			{
				m_pidMap[inputLine.pid] = std::move(handle);
				m_processMonitor.Add(inputLine.pid, inputLine.handle);
			}
			else
			{
				inputLine.handle = nullptr;
			}
		}

		// since a line can contain multiple newlines, processing 1 line can output
		// multiple lines, in this case the timestamp for each line is the same.
		auto processedLines = m_newlineFilter.Process(inputLine);
		for (auto it = processedLines.begin(); it != processedLines.end(); ++it)
		{
			boost::trim_right_if(it->message, boost::is_any_of(" \r\n\t"));
			lines.push_back(*it);
		}
	}
	return lines;
}

std::shared_ptr<DBWinReader> LogSources::AddDBWinReader(bool global)
{
	auto pDbWinReader = std::make_shared<DBWinReader>(m_timer, m_linebuffer, global);
	Add(pDbWinReader);
	return pDbWinReader;
}

std::shared_ptr<TestSource> LogSources::AddTestSource()
{
	auto pTestSource = std::make_shared<TestSource>(m_timer, m_linebuffer);
	Add(pTestSource);
	return pTestSource;
}

std::shared_ptr<ProcessReader> LogSources::AddProcessReader(const std::wstring& pathName, const std::wstring& args)
{
	auto pProcessReader = std::make_shared<ProcessReader>(m_timer, m_linebuffer, pathName, args);
	Add(pProcessReader);
	return pProcessReader;
}

// AddFileReader() is never used
std::shared_ptr<FileReader> LogSources::AddFileReader(const std::wstring& filename)
{
	auto filereader = std::make_shared<FileReader>(m_timer, m_linebuffer, IdentifyFile(filename), filename);
	Add(filereader);
	return filereader;
}

std::shared_ptr<BinaryFileReader> LogSources::AddBinaryFileReader(const std::wstring& filename)
{
	auto filetype = IdentifyFile(filename);
	AddMessage(stringbuilder() << "Started tailing " << filename << " identified as '" << FileTypeToString(filetype) << "'\n");
	auto pFileReader = std::make_shared<BinaryFileReader>(m_timer, m_linebuffer, filetype, filename);
	Add(pFileReader);
	return pFileReader;
}

// todo: DBLogReader is now always used for all types of files.
// we should choose to either rename DBLogReader, or move the FileType::AsciiText out of DBLogReader
std::shared_ptr<DBLogReader> LogSources::AddDBLogReader(const std::wstring& filename)
{
	auto filetype = IdentifyFile(filename);
	if (filetype == FileType::Unknown)
	{
		AddMessage(stringbuilder() << "Unable to open '" << filename <<"'\n");
		return std::shared_ptr<DBLogReader>();
	}
	AddMessage(stringbuilder() << "Started tailing " << filename << " identified as '" << FileTypeToString(filetype) << "'\n");

	auto pDbLogReader = std::make_shared<DBLogReader>(m_timer, m_linebuffer, filetype, filename);
	Add(pDbLogReader);
	return pDbLogReader;
}

std::shared_ptr<PipeReader> LogSources::AddPipeReader(DWORD pid, HANDLE hPipe)
{
	auto pPipeReader = std::make_shared<PipeReader>(m_timer, m_linebuffer, hPipe, pid, Str(ProcessInfo::GetProcessNameByPid(pid)).str(), 40);
	Add(pPipeReader);
	return pPipeReader;
}

std::shared_ptr<DbgviewReader> LogSources::AddDbgviewReader(const std::string& hostname)
{
	auto pDbgViewReader = std::make_shared<DbgviewReader>(m_timer, m_linebuffer, hostname);
	m_loopback->AddMessage(stringbuilder() << "Source '" << pDbgViewReader->GetDescription() << "' was added.");
	Add(pDbgViewReader);
	return pDbgViewReader;
}

std::shared_ptr<SocketReader> LogSources::AddUDPReader(const std::string& hostname, int port)
{
	auto pSocketReader = std::make_shared<SocketReader>(m_timer, m_linebuffer, hostname, port);
	m_loopback->AddMessage(stringbuilder() << "Source '" << pSocketReader->GetDescription() << "' was added.");
	Add(pSocketReader);
	return pSocketReader;
}

} // namespace debugviewpp 
} // namespace fusion
