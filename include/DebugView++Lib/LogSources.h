// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <memory>
#include <boost/signals2.hpp>
#include "Win32/Win32Lib.h"
#include "DebugView++Lib/LogSource.h"
#include "DebugView++Lib/VectorLineBuffer.h"
#include "CobaltFusion/ExecutorClient.h"
#include "DebugView++Lib/NewlineFilter.h"
#include "DebugView++Lib/ProcessMonitor.h"
#include "CobaltFusion/Throttle.h"

#pragma comment(lib, "DebugView++Lib.lib")

namespace fusion {
namespace debugviewpp {

class DBWinReader;
class ProcessReader;
class FileReader;
class AnyFileReader;
class BinaryFileReader;
class PipeReader;
class TestSource;
class Loopback;
class DbgviewReader;
class SocketReader;

typedef std::vector<HANDLE> LogSourceHandles;

template <typename T>
void EraseElements(std::vector<std::unique_ptr<T>>& v, const std::vector<T*>& e)
{
    v.erase(
        std::remove_if( // Selectively remove elements in the second vector...
            v.begin(),
            v.end(),
            [&](std::unique_ptr<T> const& p) { // This predicate checks whether the element is contained
                // in the second vector of pointers to be removed...
                return std::find(
                           e.cbegin(),
                           e.cend(),
                           p.get()) != e.cend();
            }),
        v.end());
}

class LogSources
{
public:
    typedef boost::signals2::signal<bool()> UpdateSignal;

    LogSources(IExecutor& executor, bool startListening = true);
    virtual ~LogSources();

    void SetAutoNewLine(bool value);
    bool GetAutoNewLine() const;

    virtual void SetProcessPrefix(bool value);
    virtual bool GetProcessPrefix() const;

    void ResetTimer();
    void Listen();
    void ListenUntilUpdateEvent();
    void Abort();
    bool IsRemoved(const LogSource* logsource) const;
    Lines GetLines();
    void Remove(LogSource* pLogSource);
    void RemoveSources(std::function<bool(LogSource*)> predicate);
    void CallSources(std::function<void(LogSource*)> predicate);
    void CallSources(std::function<void(LogSource*)> predicate) const;

    DBWinReader* AddDBWinReader(bool global);
    ProcessReader* AddProcessReader(const std::wstring& pathName, const std::wstring& args);
    BinaryFileReader* AddBinaryFileReader(const std::wstring& filename);
    AnyFileReader* AddAnyFileReader(const std::wstring& filename, bool keeptailing);
    DbgviewReader* AddDbgviewReader(const std::string& hostname);
    SocketReader* AddUDPReader(int port);
    PipeReader* AddPipeReader(DWORD pid, HANDLE hPipe);
    TestSource* AddTestSource(); // for unittesting
    void AddMessage(const std::string& message);
    boost::signals2::connection SubscribeToUpdate(UpdateSignal::slot_type slot);

private:
    void UpdateSources();
    void InternalRemove(LogSource*);
    void UpdateSettings(const std::unique_ptr<LogSource>& pSource);
    void Add(std::unique_ptr<LogSource> pSource);
    void OnProcessEnded(DWORD pid, HANDLE handle);
    void AddTerminateMessage(DWORD pid, HANDLE handle) const;

    mutable std::mutex m_sources_mutex;                // protects access to m_sources
    std::vector<std::unique_ptr<LogSource>> m_sources; // owned by the thread that calls Listen(), nobody else is allowed to read/write it.

    mutable std::mutex m_sourcesSchedule_mutex; // protects access to m_sourcesSchedule* vectors
    std::vector<std::unique_ptr<LogSource>> m_sourcesScheduleToAdd;
    std::vector<LogSource*> m_sourcesScheduledToRemove;

    bool m_autoNewLine = false;
    bool m_processPrefix = false;
    Win32::Handle m_updateEvent;
    bool m_end = false;
    VectorLineBuffer m_linebuffer;
    PidMap m_pidMap;
    ProcessMonitor m_processMonitor;
    NewlineFilter m_newlineFilter;
    std::unique_ptr<Loopback> m_loopback;
    Timer m_timer;

    IExecutor& m_executor;
    UpdateSignal m_update;
    Throttle m_throttledUpdate;

    // make sure this thread is last to initialize
    ActiveExecutorClient m_listenThread;
};

} // namespace debugviewpp
} // namespace fusion
