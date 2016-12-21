// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <cassert>
#include <iostream>
#include <chrono>
#include <boost/algorithm/string.hpp>
#include "CobaltFusion/stringbuilder.h"
#include "CobaltFusion/thread.h"
#include "CobaltFusion/fusionassert.h"
#include "Win32/Win32Lib.h"
#include "Win32/Utilities.h"
#include "DebugView++Lib/LogSources.h"
#include "DebugView++Lib/ProcessReader.h"
#include "DebugView++Lib/PipeReader.h"
#include "DebugView++Lib/FileReader.h"
#include "DebugView++Lib/BinaryFileReader.h"
#include "DebugView++Lib/AnyFileReader.h"
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

using namespace std::chrono_literals;

LogSources::LogSources(IExecutor& executor, bool startListening) :
	m_end(false),
	m_autoNewLine(true),
	m_updateEvent(CreateEvent(nullptr, false, false, nullptr)),
	m_linebuffer(64 * 1024),
	m_loopback(std::make_unique<Loopback>(m_timer, m_linebuffer)),
	m_executor(executor),
	m_throttledUpdate(m_executor, 25, [&] { m_update(); })
{
	m_processMonitor.ConnectProcessEnded([this](DWORD pid, HANDLE handle) { OnProcessEnded(pid, handle); });
	if (startListening)
		m_listenThread.CallAsync([this] { Listen(); });
}
	
LogSources::~LogSources()
{
	Abort();
}


void LogSources::AddMessage(const std::string& message)
{
	assert(m_executor.IsExecutorThread());
	m_loopback->AddInternal(message);
	m_throttledUpdate();
}

void LogSources::UpdateSettings(const std::unique_ptr<LogSource>& pSource)
{
	pSource->SetAutoNewLine(GetAutoNewLine());
}

void LogSources::Add(std::unique_ptr<LogSource> pSource)
{
	assert(m_executor.IsExecutorThread());
	std::lock_guard<std::mutex> lock(m_mutex);
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
	assert(m_executor.IsExecutorThread());
	auto description = pLogSource->GetDescription();
	EraseElements(m_sources, { pLogSource });
	AddMessage(stringbuilder() << "Source '" << description << "' was removed.");
}

std::vector<LogSource*> LogSources::GetSources() const
{
	assert(m_executor.IsExecutorThread());
	std::vector<LogSource*> sources;
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto& pSource : m_sources)
	{
		if (!dynamic_cast<Loopback*>(pSource.get()))
			sources.push_back(pSource.get());
	}
	return sources;
}

void LogSources::SetAutoNewLine(bool value)
{
	m_autoNewLine = value;
	for (auto& pSource : m_sources)
		pSource->SetAutoNewLine(value);
}

bool LogSources::GetAutoNewLine() const
{
	return m_autoNewLine;
}

void LogSources::Abort()
{
	if (!m_end)
	{
		assert(m_executor.IsExecutorThread());
		m_end = true;

		m_update.disconnect_all_slots();
		for (auto& pSource : m_sources)
		{
			pSource->Abort();
		}
	}
	Win32::SetEvent(m_updateEvent);
	m_listenThread.Synchronize();
}

void LogSources::Reset()
{
	m_timer.Reset();
}

boost::signals2::connection LogSources::SubscribeToUpdate(Update::slot_type slot)
{
	return m_update.connect(slot);
}

// default behaviour: 
// LogSources starts with 1 logsource, the loopback source
// At startup normally 1 DBWinReader is added by m_logSources.AddDBWinReader
void LogSources::Listen()
{
	try
	{
		ListenUntilUpdateEvent();
		if (!m_end) 
			m_listenThread.CallAsync([this] { Listen(); });
	}
	catch (const std::exception& e)
	{
		FUSION_REPORT_EXCEPTION(e.what());
		throw;
	}
}

void LogSources::ListenUntilUpdateEvent()
{
	std::vector<HANDLE> waitHandles;
	std::vector<LogSource*> sources;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		for (auto& source : m_sources)
		{
			if (source->AtEnd()) continue;
			HANDLE handle = source->GetHandle();
			if (handle != INVALID_HANDLE_VALUE)
			{
				waitHandles.push_back(handle);
				sources.push_back(source.get());
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
				m_throttledUpdate();
			}
		}
	}
	m_executor.Call([this] { UpdateSources(); });
}

void LogSources::UpdateSources()
{
	assert(m_executor.IsExecutorThread());
	std::vector<LogSource*> sources;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		for (auto& pSource : m_sources)
		{
			sources.push_back(pSource.get());
		}
	}

	for (auto& pSource : sources)
	{
		if (pSource->AtEnd())
		{
			pSource->Abort();
			m_update();
			InternalRemove(pSource);
		}
	}
}

void LogSources::OnProcessEnded(DWORD pid, HANDLE handle)
{
	m_executor.CallAsync([this, pid, handle]
	{
		m_update();
		auto flushedLines = m_newlineFilter.FlushLinesFromTerminatedProcess(pid, handle);
		for (auto& line : flushedLines)
			m_loopback->Add(line.pid, line.processName, line.message);
		m_throttledUpdate();
		auto it = m_pidMap.find(pid);
		if (it != m_pidMap.end())
			m_pidMap.erase(it);
	});
}

Lines LogSources::GetLines()
{
	assert(m_executor.IsExecutorThread());
	auto inputLines = m_linebuffer.GetLines();

	Lines lines;
	for (auto& inputLine : inputLines)
	{
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

		if (inputLine.message.empty())
		{
			lines.push_back(inputLine);
		}
		else
		{
			// since a line can contain multiple newlines, processing 1 line can output
			// multiple lines, in this case the timestamp for each line is the same.
			// NewlineFilter::Process will also eat any \r\n's
			auto processedLines = m_newlineFilter.Process(inputLine);
			for (auto& line : processedLines)
			{
				lines.push_back(line);
			}
		}
	}
	return lines;
}

DBWinReader* LogSources::AddDBWinReader(bool global)
{
	assert(m_executor.IsExecutorThread());
	auto pDbWinReader = std::make_unique<DBWinReader>(m_timer, m_linebuffer, global);
	auto pResult = pDbWinReader.get();
	Add(std::move(pDbWinReader));
	return pResult;
}

TestSource* LogSources::AddTestSource()
{
	assert(m_executor.IsExecutorThread());
	auto pTestSource = std::make_unique<TestSource>(m_timer, m_linebuffer);
	auto pResult = pTestSource.get();
	Add(std::move(pTestSource));
	return pResult;
}

ProcessReader* LogSources::AddProcessReader(const std::wstring& pathName, const std::wstring& args)
{
	assert(m_executor.IsExecutorThread());
	auto pProcessReader = std::make_unique<ProcessReader>(m_timer, m_linebuffer, pathName, args);
	auto pResult = pProcessReader.get();
	Add(std::move(pProcessReader));
	return pResult;
}

BinaryFileReader* LogSources::AddBinaryFileReader(const std::wstring& filename)
{
	assert(m_executor.IsExecutorThread());
	auto filetype = IdentifyFile(filename);
	AddMessage(stringbuilder() << "Started tailing " << filename << " identified as '" << FileTypeToString(filetype) << "'\n");
	auto pFileReader = std::make_unique<BinaryFileReader>(m_timer, m_linebuffer, filetype, filename);
	auto pResult = pFileReader.get();
	Add(std::move(pFileReader));
	return pResult;
}

AnyFileReader* LogSources::AddAnyFileReader(const std::wstring& filename, bool keeptailing)
{
	assert(m_executor.IsExecutorThread());
	auto filetype = IdentifyFile(filename);
	if (filetype == FileType::Unknown)
	{
		AddMessage(stringbuilder() << "Unable to open '" << filename <<"'\n");
		return nullptr;
	}

	if (keeptailing)
	{
		AddMessage(stringbuilder() << "Started tailing " << filename << " identified as '" << FileTypeToString(filetype) << "'\n");
	}
	else
	{
		AddMessage(stringbuilder() << "Loading " << filename << " identified as '" << FileTypeToString(filetype) << "'\n");
	}

	auto pAnyFileReader = std::make_unique<AnyFileReader>(m_timer, m_linebuffer, filetype, filename, keeptailing);
	auto pResult = pAnyFileReader.get();
	Add(std::move(pAnyFileReader));
	return pResult;
}

PipeReader* LogSources::AddPipeReader(DWORD pid, HANDLE hPipe)
{
	assert(m_executor.IsExecutorThread());
	auto pPipeReader = std::make_unique<PipeReader>(m_timer, m_linebuffer, hPipe, pid, Str(ProcessInfo::GetProcessNameByPid(pid)).str(), 40);
	auto pResult = pPipeReader.get();
	Add(std::move(pPipeReader));
	return pResult;
}

DbgviewReader* LogSources::AddDbgviewReader(const std::string& hostname)
{
	assert(m_executor.IsExecutorThread());
	auto pDbgViewReader = std::make_unique<DbgviewReader>(m_timer, m_linebuffer, hostname);
	m_loopback->Add(stringbuilder() << "Source '" << pDbgViewReader->GetDescription() << "' was added.");
	auto pResult = pDbgViewReader.get();
	Add(std::move(pDbgViewReader));
	return pResult;
}

SocketReader* LogSources::AddUDPReader(int port)
{
	assert(m_executor.IsExecutorThread());
	auto pSocketReader = std::make_unique<SocketReader>(m_timer, m_linebuffer, port);
	m_loopback->Add(stringbuilder() << "Source '" << pSocketReader->GetDescription() << "' was added.");
	auto pResult = pSocketReader.get();
	Add(std::move(pSocketReader));
	return pResult;
}

} // namespace debugviewpp 
} // namespace fusion
