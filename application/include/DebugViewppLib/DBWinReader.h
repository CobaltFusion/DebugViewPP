// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <atomic>
#include <chrono>
#include "Win32/Win32Lib.h"
#include "LogSource.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer;
class ProcessCommandLines;

struct DBWinMessage
{
    double time;
    FILETIME systemTime;
    DWORD pid;
    std::string message;
    HANDLE handle;
};

struct DbWinBuffer;

class DBWinReader : public LogSource
{
public:
    // when pCommandLines is given, the commandline of every process that sends a message is stored in it,
    // this must be done here while the sender is still waiting for the buffer to be released, so the process is certainly alive.
    DBWinReader(Timer& timer, ILineBuffer& lineBuffer, bool global, ProcessCommandLines* pCommandLines = nullptr);
    ~DBWinReader() override;

    HANDLE GetHandle() const override;
    void Notify() override;

    // Delays the process that calls OutputDebugString(): a sender waits for DBWIN_BUFFER_READY before it can write its
    // message, by signalling that event later, every call is slowed down by (at least) the given time.
    // Useful to slow a process down on purpose while debugging. Zero (the default) means no delay.
    void SetDelay(std::chrono::milliseconds delay);
    std::chrono::milliseconds GetDelay() const;

private:
    Win32::Handle m_hBuffer;
    Win32::Handle m_dbWinBufferReady;
    Win32::Handle m_dbWinDataReady;
    Win32::MappedViewOfFile m_mappedViewOfFile;
    const DbWinBuffer* m_dbWinBuffer;
    ProcessCommandLines* m_pCommandLines;
    std::atomic<long long> m_delayMs{0};
    PTP_TIMER m_delayTimer = nullptr;
};

} // namespace debugviewpp
} // namespace fusion
