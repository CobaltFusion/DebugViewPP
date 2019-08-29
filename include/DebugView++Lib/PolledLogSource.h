// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <thread>
#include "Win32/Win32Lib.h"
#include "LogSource.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

struct PollLine
{
    PollLine(Win32::Handle handle, const std::string& message, const LogSource* pLogSource);
    PollLine(DWORD pid, const std::string& processName, const std::string& message, const LogSource* pLogSource);
    PollLine(double time, FILETIME systemTime, DWORD pid, const std::string& processName, const std::string& message, const LogSource* pLogSource);

    bool timesValid;
    double time;
    FILETIME systemTime;
    Win32::Handle handle;
    DWORD pid;
    std::string processName;
    std::string message;
    const LogSource* pLogSource;
};

class PolledLogSource : public LogSource
{
public:
    PolledLogSource(Timer& timer, SourceType::type sourceType, ILineBuffer& lineBuffer, long pollFrequency);
    ~PolledLogSource() override;

    HANDLE GetHandle() const override;
    void Notify() override;
    virtual void Poll();
    void Abort() override;

    // in contrast to the LogSource::Add methdods, these methods are de-coupled using m_backBuffer so they
    // can be used to add messages from any thread. The typical use-case are messages from the UI thread.
    void AddMessage(Win32::Handle handle, const std::string& message);
    void AddMessage(DWORD pid, const std::string& processName, const std::string& message);
    void AddMessage(const std::string& message);
    void AddMessage(double time, FILETIME systemTime, DWORD pid, const std::string& processName, const std::string& message);

    void Signal();
    void StartThread();

    long GetMicrosecondInterval() const;

private:
    void Loop();

    std::vector<PollLine> m_lines;
    std::vector<PollLine> m_backBuffer;
    std::chrono::microseconds m_microsecondInterval;
    Win32::Handle m_handle;
    std::mutex m_mutex;
    std::unique_ptr<std::thread> m_thread;
};

} // namespace debugviewpp
} // namespace fusion
