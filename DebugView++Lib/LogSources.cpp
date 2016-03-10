// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <cassert>
#include <boost/algorithm/string.hpp>
#include "CobaltFusion/stringbuilder.h"
#include "Win32/Win32Lib.h"
#include "Win32/Utilities.h"
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
#include "CobaltFusion/make_unique.h"

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
	m_loopback(CreateLoopback(m_timer, m_linebuffer)),
	m_updatePending(false)
{
	
	if (startListening)
		m_listenThread = boost::thread(&LogSources::Listen, this);
	m_processMonitor.ConnectProcessEnded([this](DWORD pid, HANDLE handle) { OnProcessEnded(pid, handle); });
}
	
LogSources::~LogSources()
{
	Abort();
}

Loopback* LogSources::CreateLoopback(Timer& timer, ILineBuffer& lineBuffer)
{
	auto loopback = make_unique<Loopback>(timer, lineBuffer);
	auto result = loopback.get();
	m_sources.emplace_back(std::move(loopback));
	return result;
}

void LogSources::AddMessage(const std::string& message)
{
	m_loopback->AddMessage(message);
	m_loopback->Signal();
}

void LogSources::UpdateSettings(const std::unique_ptr<LogSource>& pSource)
{
	pSource->SetAutoNewLine(GetAutoNewLine());
}

void LogSources::Add(std::unique_ptr<LogSource> pSource)
{
	assert(m_guiExecutor.IsExecutorThread());
	boost::mutex::scoped_lock lock(m_mutex);
	UpdateSettings(pSource);
	m_sources.emplace_back(std::move(pSource));
	Win32::SetEvent(m_updateEvent);
}

void LogSources::Remove(LogSource* pLogSource)
{
	pLogSource->Abort();
	Win32::SetEvent(m_updateEvent);
}

void LogSources::InternalRemove(LogSource* pLogSource)
{
	assert(m_guiExecutor.IsExecutorThread());
	AddMessage(stringbuilder() << "Source '" << pLogSource->GetDescription() << "' was removed.");
	boost::mutex::scoped_lock lock(m_mutex);
	pLogSource->Abort();
	std::vector<LogSource*> v;
	v.push_back(pLogSource);
	EraseElements(m_sources, v);
}

std::vector<LogSource*> LogSources::GetSources() const
{
	std::vector<LogSource*> sources;
	boost::mutex::scoped_lock lock(m_mutex);
	for (auto& it = m_sources.begin(); it != m_sources.end(); ++it)
	{
		if (dynamic_cast<Loopback*>(it->get()))
			sources.push_back(it->get());
	}
	return sources;
}

void LogSources::SetAutoNewLine(bool value)
{
	m_autoNewLine = value;
	for (auto& it = m_sources.begin(); it != m_sources.end(); ++it)
		(*it)->SetAutoNewLine(value);
}

bool LogSources::GetAutoNewLine() const
{
	return m_autoNewLine;
}

void LogSources::Abort()
{
	m_end = true;

	for (auto& it = m_sources.begin(); it != m_sources.end(); ++it)
	{
		(*it)->Abort();
	}
	Win32::SetEvent(m_updateEvent);
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
	m_updatePending = true;
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
		m_updatePending = false;
		// schedule one more update to workaround the race-condition writing to m_updatePending
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
	std::vector<LogSource*> sources;
	{
		boost::mutex::scoped_lock lock(m_mutex);
		for (auto& it = m_sources.begin(); it != m_sources.end(); ++it)
		{
			auto source = it->get();
			if (source->AtEnd()) continue;
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
		auto res = Win32::WaitForAnyObject(waitHandles, INFINITE);
		if (m_end) return;

		if (res.signaled)
		{
			int index = res.index - WAIT_OBJECT_0;
			if (index == updateEventIndex)
				break;
			else
			{
				assert((index < static_cast<int>(sources.size())) && "res.index out of range");
				auto logsource = sources[index];
				logsource->Notify();
				if (!m_updatePending)
					OnUpdate();
			}
		}
	}
	m_guiExecutor.Call([this] { UpdateSources(); });
}

void LogSources::UpdateSources()
{
	std::vector<LogSource*> sources;
	{
		boost::mutex::scoped_lock lock(m_mutex);
		for (auto& it = m_sources.begin(); it != m_sources.end(); ++it)
		{
			sources.push_back(it->get());
		}
	}

	for (auto it = sources.begin(); it != sources.end(); ++it)
	{
		if ((*it)->AtEnd())
		{
			InternalRemove(*it);
		}
	}
}

void LogSources::OnProcessEnded(DWORD pid, HANDLE handle)
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

bool LogSources::LogSourceExists(const LogSource* pLogSource) const
{
	for (auto it = m_sources.begin(); it != m_sources.end(); ++it)
	{
		auto source = it->get();
		if (pLogSource == source) return true;
	}
	return false;
}

Lines LogSources::GetLines()
{
	auto inputLines = m_linebuffer.GetLines();
	Lines lines;
	for (auto it = inputLines.begin(); it != inputLines.end(); ++it)
	{
		auto& inputLine = *it;
		if (!LogSourceExists(inputLine.pLogSource)) continue;

		// let the logsource decide how to create processname
		if (inputLine.pLogSource)
		{
			inputLine.pLogSource->PreProcess(inputLine);
		}

		if (inputLine.handle)
		{
			Win32::Handle handle(inputLine.handle);
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

DBWinReader* LogSources::AddDBWinReader(bool global)
{
	auto pDbWinReader = make_unique<DBWinReader>(m_timer, m_linebuffer, global);
	auto pResult = pDbWinReader.get();
	Add(std::move(pDbWinReader));
	return pResult;
}

TestSource* LogSources::AddTestSource()
{
	auto pTestSource = make_unique<TestSource>(m_timer, m_linebuffer);
	auto pResult = pTestSource.get();
	Add(std::move(pTestSource));
	return pResult;
}

ProcessReader* LogSources::AddProcessReader(const std::wstring& pathName, const std::wstring& args)
{
	auto pProcessReader = make_unique<ProcessReader>(m_timer, m_linebuffer, pathName, args);
	auto pResult = pProcessReader.get();
	Add(std::move(pProcessReader));
	return pResult;
}

// AddFileReader() is never used
FileReader* LogSources::AddFileReader(const std::wstring& filename)
{
	auto pFilereader = make_unique<FileReader>(m_timer, m_linebuffer, IdentifyFile(filename), filename);
	auto pResult = pFilereader.get();
	Add(std::move(pFilereader));
	return pResult;
}

BinaryFileReader* LogSources::AddBinaryFileReader(const std::wstring& filename)
{
	auto filetype = IdentifyFile(filename);
	AddMessage(stringbuilder() << "Started tailing " << filename << " identified as '" << FileTypeToString(filetype) << "'\n");
	auto pFileReader = make_unique<BinaryFileReader>(m_timer, m_linebuffer, filetype, filename);
	auto pResult = pFileReader.get();
	Add(std::move(pFileReader));
	return pResult;
}

// todo: DBLogReader is now always used for all types of files.
// we should choose to either rename DBLogReader, or move the FileType::AsciiText out of DBLogReader
DBLogReader* LogSources::AddDBLogReader(const std::wstring& filename)
{
	auto filetype = IdentifyFile(filename);
	if (filetype == FileType::Unknown)
	{
		AddMessage(stringbuilder() << "Unable to open '" << filename <<"'\n");
		return nullptr;
	}
	AddMessage(stringbuilder() << "Started tailing " << filename << " identified as '" << FileTypeToString(filetype) << "'\n");

	auto pDbLogReader = make_unique<DBLogReader>(m_timer, m_linebuffer, filetype, filename);
	auto pResult = pDbLogReader.get();
	Add(std::move(pDbLogReader));
	return pResult;
}

PipeReader* LogSources::AddPipeReader(DWORD pid, HANDLE hPipe)
{
	auto pPipeReader = make_unique<PipeReader>(m_timer, m_linebuffer, hPipe, pid, Str(ProcessInfo::GetProcessNameByPid(pid)).str(), 40);
	auto pResult = pPipeReader.get();
	Add(std::move(pPipeReader));
	return pResult;
}

DbgviewReader* LogSources::AddDbgviewReader(const std::string& hostname)
{
	auto pDbgViewReader = make_unique<DbgviewReader>(m_timer, m_linebuffer, hostname);
	m_loopback->AddMessage(stringbuilder() << "Source '" << pDbgViewReader->GetDescription() << "' was added.");
	auto pResult = pDbgViewReader.get();
	Add(std::move(pDbgViewReader));
	return pResult;
}

SocketReader* LogSources::AddUDPReader(int port)
{
	auto pSocketReader = make_unique<SocketReader>(m_timer, m_linebuffer, port);
	m_loopback->AddMessage(stringbuilder() << "Source '" << pSocketReader->GetDescription() << "' was added.");
	auto pResult = pSocketReader.get();
	Add(std::move(pSocketReader));
	return pResult;
}

} // namespace debugviewpp 
} // namespace fusion
