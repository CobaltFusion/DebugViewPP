// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include "DebugViewppLib/DBWinBuffer.h"
#include "DebugViewppLib/DBWinReader.h"
#include "DebugViewppLib/ProcessInfo.h"
#include "DebugViewppLib/LineBuffer.h"
#include "CobaltFusion/stringbuilder.h"

namespace fusion {
namespace debugviewpp {

const char* systemProcessNames[] = {"SYSTEM_PID_0", "SYSTEM_PID_1", "SYSTEM_PID_2", "SYSTEM_PID_3", "SYSTEM_PID_4"};
constexpr int systemProcessNamesCount = sizeof(systemProcessNames) / sizeof(systemProcessNames[0]);

std::wstring GetDBWinName(bool global, const std::wstring& name)
{
    return global ? L"Global\\" + name : name;
}

Win32::Handle CreateDBWinBufferMapping(bool global)
{
    auto name = GetDBWinName(global, L"DBWIN_BUFFER");
    Win32::Handle hMap(Win32::CreateFileMapping(nullptr, nullptr, PAGE_READWRITE, 0, sizeof(DbWinBuffer), name.c_str()));
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        throw std::runtime_error("CreateDBWinBufferMapping");
    }
    return hMap;
}

namespace {

// context is the DBWIN_BUFFER_READY event
VOID CALLBACK SignalBufferReady(PTP_CALLBACK_INSTANCE /*instance*/, PVOID context, PTP_TIMER /*timer*/)
{
    ::SetEvent(static_cast<HANDLE>(context));
}

} // namespace

DBWinReader::DBWinReader(Timer& timer, ILineBuffer& linebuffer, bool global) :
    LogSource(timer, SourceType::System, linebuffer),
    m_hBuffer(CreateDBWinBufferMapping(global)),

    m_dbWinBufferReady(Win32::CreateEvent(nullptr, false, true, GetDBWinName(global, L"DBWIN_BUFFER_READY").c_str())),
    m_dbWinDataReady(Win32::CreateEvent(nullptr, false, false, GetDBWinName(global, L"DBWIN_DATA_READY").c_str())),
    m_mappedViewOfFile(m_hBuffer.get(), PAGE_READONLY, 0, 0, sizeof(DbWinBuffer)),
    m_dbWinBuffer(static_cast<const DbWinBuffer*>(m_mappedViewOfFile.Ptr()))
{
    SetDescription(global ? L"Global Win32 Messages" : L"Win32 Messages");

    // required permissions for messages from UWP applications
    Win32::AdjustObjectDACL(m_hBuffer.get());
    Win32::AdjustObjectDACL(m_dbWinBufferReady.get());
    Win32::AdjustObjectDACL(m_dbWinDataReady.get());

    m_delayTimer = ::CreateThreadpoolTimer(&SignalBufferReady, m_dbWinBufferReady.get(), nullptr);
    if (m_delayTimer == nullptr)
    {
        Win32::ThrowLastError("CreateThreadpoolTimer");
    }

    Win32::SetEvent(m_dbWinBufferReady);
}

DBWinReader::~DBWinReader()
{
    // cancel a pending delay and wait for a running callback, the callback uses the m_dbWinBufferReady event.
    ::SetThreadpoolTimer(m_delayTimer, nullptr, 0, 0);
    ::WaitForThreadpoolTimerCallbacks(m_delayTimer, TRUE);
    ::CloseThreadpoolTimer(m_delayTimer);

    // do not leave a sender waiting for a buffer that is never going to be read
    ::SetEvent(m_dbWinBufferReady.get());
}

void DBWinReader::SetDelay(std::chrono::milliseconds delay)
{
    m_delayMs = std::max<long long>(delay.count(), 0);
}

std::chrono::milliseconds DBWinReader::GetDelay() const
{
    return std::chrono::milliseconds(m_delayMs.load());
}

HANDLE DBWinReader::GetHandle() const
{
    return m_dbWinDataReady.get();
}

// by forcing Notify() to be not inlined, it will have a better change of being aligned correctly
// in the instruction cache. __declspec(noinline), however measurements do not show any differences
void DBWinReader::Notify()
{
    static_assert(systemProcessNamesCount == Win32::fixedNumberOfSystemPids, "The size of the systemProcessNames array must be 'fixedNumberOfSystemPids' (5)");
    if (m_dbWinBuffer->processId < Win32::fixedNumberOfSystemPids)
    {
        Add(m_dbWinBuffer->processId, systemProcessNames[m_dbWinBuffer->processId], m_dbWinBuffer->data);
    }
    else
    {
        HANDLE handle = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, m_dbWinBuffer->processId);
#ifdef OPENPROCESS_DEBUG
        if (!handle)
        {
            Win32::Win32Error error(GetLastError(), "OpenProcess");
            LogSource::Add(stringbuilder() << error.what() << ", data: " << m_dbWinBuffer->data << " (pid: " << m_dbWinBuffer->processId << ")");
        }
#else
        // performance does not improve significantly (almost immeasurable) without the Add() call, nor without the OpenProcess call.
        // there might be performance to be gained by:
        // - making a non-virtual Notice() method
        // - copying the information from m_dbWinBuffer->data and calling SetEvent(m_dbWinBufferReady.get()); before _any_ other operation.
        // - check the m_dbWinDataReady is already set again after Add()
        if (handle != nullptr)
        {
            Add(handle, m_dbWinBuffer->data);
        }
        else
        {
            Add(m_dbWinBuffer->processId, "<system>", m_dbWinBuffer->data);
        }
#endif
    }

    // signalling the buffer is ready lets the sender continue, so delaying it delays the sender.
    // (only one message is in flight at a time, the sender can not write again until the event is set, so one timer is enough)
    const auto delayMs = m_delayMs.load();
    if (delayMs <= 0)
    {
        ::SetEvent(m_dbWinBufferReady.get());
        return;
    }

    LARGE_INTEGER relativeDueTime; // negative means relative, in 100ns units
    relativeDueTime.QuadPart = -delayMs * 10'000;
    FILETIME dueTime;
    dueTime.dwLowDateTime = relativeDueTime.LowPart;
    dueTime.dwHighDateTime = static_cast<DWORD>(relativeDueTime.HighPart);
    ::SetThreadpoolTimer(m_delayTimer, &dueTime, 0, 0);
}

} // namespace debugviewpp
} // namespace fusion
