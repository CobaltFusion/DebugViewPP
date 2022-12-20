// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <unordered_map>

#include "windows.h"

#pragma comment(lib, "DebugView++Lib.lib")

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

    DWORD GetUid(DWORD processId, const std::wstring& processName);
    ProcessProperties GetProcessProperties(DWORD processId, const std::wstring& processName);
    ProcessProperties GetProcessProperties(DWORD uid) const;

private:
    std::unordered_map<DWORD, InternalProcessProperties> m_processProperties;
    DWORD m_unqiueId;
};

} // namespace debugviewpp
} // namespace fusion
