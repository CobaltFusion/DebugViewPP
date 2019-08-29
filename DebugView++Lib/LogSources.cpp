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
    m_updateEvent(CreateEvent(nullptr, 0, 0, nullptr)),
    m_linebuffer(64 * 1024),
    m_loopback(std::make_unique<Loopback>(m_timer, m_linebuffer)),
    m_executor(executor),
    m_throttledUpdate(m_executor, 25, [&] { m_update(); })
{
    m_processMonitor.ConnectProcessEnded([this](DWORD pid, HANDLE handle) { OnProcessEnded(pid, handle); });
    if (startListening)
    {
        m_listenThread.CallAsync([this] { Listen(); });
    }
}

LogSources::~LogSources()
{
    if (m_executor.IsExecutorThread())
    { // this is true when the GuiExector is used
        Abort();
    }
    else
    {
        m_executor.Call([&] { Abort(); });
    }
}

void LogSources::AddMessage(const std::string& message)
{
    m_loopback->AddInternal(message);
    m_throttledUpdate();
}

void LogSources::UpdateSettings(const std::unique_ptr<LogSource>& pSource)
{
    pSource->SetAutoNewLine(GetAutoNewLine());
}

void LogSources::Add(std::unique_ptr<LogSource> pSource)
{
    {
        std::lock_guard<std::mutex> lock(m_sourcesSchedule_mutex);
        m_sourcesScheduleToAdd.emplace_back(std::move(pSource));
    }
    Win32::SetEvent(m_updateEvent);
}

void LogSources::InternalRemove(LogSource* pLogSource)
{
    auto description = pLogSource->GetDescription();
    EraseElements(m_sources, {pLogSource});
    AddMessage(stringbuilder() << "Source '" << description << "' was removed.");
}

void LogSources::Remove(LogSource* pLogSource)
{
    m_sourcesScheduledToRemove.push_back(pLogSource);
    Win32::SetEvent(m_updateEvent);
}

void LogSources::RemoveSources(std::function<bool(LogSource*)> predicate)
{
    std::lock_guard<std::mutex> lock(m_sourcesSchedule_mutex);
    for (auto& pSource : m_sources)
    {
        if (predicate(pSource.get()))
        {
            m_sourcesScheduledToRemove.push_back(pSource.get());
        }
    }
    Win32::SetEvent(m_updateEvent);
}

void LogSources::CallSources(std::function<void(LogSource*)> predicate)
{
    std::lock_guard<std::mutex> lock(m_sourcesSchedule_mutex);
    for (auto& pSource : m_sources)
    {
        predicate(pSource.get());
    }
}

void LogSources::CallSources(std::function<void(LogSource*)> predicate) const
{
    std::lock_guard<std::mutex> lock(m_sourcesSchedule_mutex);
    for (auto& pSource : m_sources)
    {
        predicate(pSource.get());
    }
}


void LogSources::SetAutoNewLine(bool value)
{
    m_autoNewLine = value;
    for (auto& pSource : m_sources)
    {
        pSource->SetAutoNewLine(value);
    }
}

bool LogSources::GetAutoNewLine() const
{
    return m_autoNewLine;
}

void LogSources::SetProcessPrefix(bool value)
{
    m_processPrefix = value;
}

bool LogSources::GetProcessPrefix() const
{
    return m_processPrefix;
}

void LogSources::Abort()
{
    m_processMonitor.Abort();
    m_update.disconnect_all_slots();
    m_end = true;

    CallSources([](LogSource* logsource) { logsource->Abort(); });
    RemoveSources([](LogSource* /*unused*/) { return true; });
    Win32::SetEvent(m_updateEvent);
    m_listenThread.Synchronize();
}

void LogSources::ResetTimer()
{
    m_timer.Reset();
}

boost::signals2::connection LogSources::SubscribeToUpdate(UpdateSignal::slot_type slot)
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
        {
            m_listenThread.CallAsync([this] { Listen(); });
        }
    }
    catch (const std::exception& e)
    {
        FUSION_REPORT_EXCEPTION(e.what());
        throw;
    }
}

void LogSources::ListenUntilUpdateEvent()
{
    UpdateSources();

    std::vector<HANDLE> waitHandles;
    std::vector<LogSource*> sources;
    {
        for (auto& source : m_sources)
        {
            if (source->AtEnd())
            {
                continue;
            }
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
        if (m_end)
        {
            return;
        }

        if (res.signaled)
        {
            int index = res.index - WAIT_OBJECT_0;
            if (index == updateEventIndex)
            {
                break;
            }

            assert((index < static_cast<int>(sources.size())) && "res.index out of range");
            auto logsource = sources[index];
            logsource->Notify();
            m_throttledUpdate();
        }
    }
}

void LogSources::UpdateSources()
{
    std::vector<std::unique_ptr<LogSource>> sourcesToAdd;
    std::vector<LogSource*> sourcesToRemove;
    {
        std::lock_guard<std::mutex> lock(m_sourcesSchedule_mutex);
        std::swap(sourcesToAdd, m_sourcesScheduleToAdd);
        std::swap(sourcesToRemove, m_sourcesScheduledToRemove);
    }

    if (m_end)
    {
        for (auto const& pLogSource : m_sources)
        {
            pLogSource->Abort();
        }
        m_sources.clear();
        return;
    }

    for (auto pLogSource : sourcesToRemove)
    {
        pLogSource->Abort();
        InternalRemove(pLogSource);
    }

    for (auto& pLogSource : sourcesToAdd)
    {
        UpdateSettings(pLogSource);
        m_sources.emplace_back(std::move(pLogSource));
    }

    m_throttledUpdate(); // notify observers to process internal messages
}

std::string FormatExitCode(DWORD exitCode)
{
    auto longCode = static_cast<long>(exitCode);
    if (std::abs(longCode) < 16)
    {
        if (longCode < 0)
        {
            return stringbuilder() << "exit code " << longCode << " (" << std::hex << std::showbase << std::internal << exitCode << ")";
        }
        return stringbuilder() << "exit code " << longCode;
    }
    if (exitCode >= 0xC0000000) // >= 0xC0000000 in case of an SEH exception
    {
        return stringbuilder() << std::hex << std::showbase << std::internal << exitCode << " (" << Win32::GetSEHcodeDescription(exitCode) << ")";
    }
    if (exitCode >= 0x80000000) // >= 0x80000000 in case of a HRESULT rethrown as an SEH
    {
        return stringbuilder() << "HRESULT " << std::hex << std::showbase << std::internal << exitCode << " (" << Win32::GetHresultDescription(exitCode) << ")";
    }
    return stringbuilder() << "exit code " << longCode << " (" << std::hex << std::showbase << std::internal << exitCode << ")";
}

void LogSources::AddTerminateMessage(DWORD pid, HANDLE handle) const
{
    auto processName = Str(ProcessInfo::GetProcessName(handle)).str();
    auto startTime = ProcessInfo::GetStartTime(handle);

    std::string terminateMessage = stringbuilder() << "<process started at " << startTime << " has terminated";
    DWORD exitCode = 0;
    auto result = ::GetExitCodeProcess(handle, &exitCode);
    if (result == 0)
    {
        terminateMessage += " with unknown exit code";
    }
    else
    {
        terminateMessage += " with ";
        terminateMessage += FormatExitCode(exitCode);
    }
    terminateMessage += ">";
    m_loopback->Add(pid, processName, terminateMessage);
}

void LogSources::OnProcessEnded(DWORD pid, HANDLE handle)
{
    m_executor.CallAsync([this, pid, handle] {
        m_update();
        auto flushedLines = m_newlineFilter.FlushLinesFromTerminatedProcess(pid, handle);
        for (auto& line : flushedLines)
        {
            m_loopback->Add(line.pid, line.processName, line.message);
        }
        AddTerminateMessage(pid, handle);
        m_throttledUpdate();
        auto it = m_pidMap.find(pid);
        if (it != m_pidMap.end())
        {
            m_pidMap.erase(it);
        }
    });
}

bool LogSources::IsRemoved(const LogSource* logsource) const
{
    bool present = false;
    CallSources([&](LogSource* l) { if (l == logsource) { present = true; 
} });
    return !present;
}

Lines LogSources::GetLines()
{
    assert(m_executor.IsExecutorThread());
    Lines lines;
    for (auto&& inputLine : m_linebuffer.GetLines())
    {
        if (IsRemoved(inputLine.pLogSource))
        {
            continue;
        }
        // let the logsource decide how to create processname
        if (inputLine.pLogSource != nullptr)
        {
            inputLine.pLogSource->PreProcess(inputLine);
        }

        if (inputLine.handle != nullptr)
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
            lines.emplace_back(std::move(inputLine));
        }
        else
        {
            // since a line can contain multiple newlines, processing 1 line can output
            // multiple lines, in this case the timestamp for each line is the same.
            // NewlineFilter::Process will also eat any \r\n's

            for (auto&& line : m_newlineFilter.Process(std::move(inputLine)))
            {
                lines.emplace_back(std::move(line));
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
    pFileReader->SubscribeToUpdate([&]() { m_throttledUpdate(); });

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
        AddMessage(stringbuilder() << "Unable to open '" << filename << "'\n");
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
    pAnyFileReader->SubscribeToUpdate([&]() { m_throttledUpdate(); });
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
