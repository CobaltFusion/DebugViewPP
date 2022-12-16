// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#pragma once

#include <string>
#include <vector>
#include "Win32/Win32Lib.h"
#include <psapi.h>

namespace fusion {
namespace Win32 {

std::wstring GetModuleFilename();
std::wstring GetExecutionPath();
std::wstring GetModuleFilenameUnspoofable();

class Process
{
public:
    Process(const std::wstring& pathName, const std::vector<std::wstring>& args);
    Process(const std::wstring& pathName, const std::wstring& args);

    std::wstring GetName() const;
    HANDLE GetStdIn() const;
    HANDLE GetStdOut() const;
    HANDLE GetStdErr() const;
    HANDLE GetProcessHandle() const;
    HANDLE GetThreadHandle() const;
    unsigned long GetProcessId() const;
    unsigned long GetThreadId() const;

    bool IsRunning() const;
    void Wait() const;

private:
    void Run(const std::wstring& pathName, const std::wstring& args);

    std::wstring m_name;
    Handle m_stdIn;
    Handle m_stdOut;
    Handle m_stdErr;
    Handle m_hProcess;
    Handle m_hThread;
    unsigned long m_processId;
    unsigned long m_threadId;
};

} // namespace Win32
} // namespace fusion
