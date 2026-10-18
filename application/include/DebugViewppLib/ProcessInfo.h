// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

#include "windows.h"

namespace fusion {
namespace debugviewpp {

struct InternalProcessProperties
{
    InternalProcessProperties();
    InternalProcessProperties(DWORD pid, const std::wstring& name, COLORREF color);

    DWORD pid; // system processId
    std::wstring name;
    COLORREF color;
};

struct ProcessProperties
{
    explicit ProcessProperties(const InternalProcessProperties& iprops);

    DWORD uid; // unique id
    DWORD pid; // system processId
    std::wstring name;
    COLORREF color;
};

class ProcessInfo
{
public:
    ProcessInfo();
    void Clear();
    static size_t GetPrivateBytes();
    static std::wstring GetProcessName(HANDLE handle);
    static std::wstring GetStartTime(HANDLE handle);
    static std::wstring GetProcessNameByPid(DWORD processId);
    // returns an empty string when the commandline is not available (needs Windows 8.1 or later, and a process that is still running)
    static std::wstring GetProcessCommandLine(HANDLE handle);

    DWORD GetUid(DWORD processId, const std::wstring& processName);
    ProcessProperties GetProcessProperties(DWORD processId, const std::wstring& processName);
    ProcessProperties GetProcessProperties(DWORD uid) const;

private:
    std::unordered_map<DWORD, InternalProcessProperties> m_processProperties;
    DWORD m_unqiueId;
};

// The commandline of a process can only be read while the process is running, but it is most useful when the process
// has terminated: it is stored when the first message of a process is received and taken when the process terminates.
// Thread safe, the messages are received on another thread than where the termination is handled.
class ProcessCommandLines
{
public:
    bool Has(DWORD pid) const;
    void Set(DWORD pid, const std::wstring& commandLine);
    std::wstring Take(DWORD pid); // removes the entry, returns an empty string if there is none

private:
    mutable std::mutex m_mutex;
    std::unordered_map<DWORD, std::wstring> m_commandLines;
};

} // namespace debugviewpp
} // namespace fusion
