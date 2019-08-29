// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <cassert>
#include <array>
#include <psapi.h>
#include "CobaltFusion/Str.h"
#include "Win32/Win32Lib.h"
#include "DebugView++Lib/Conversions.h"
#include "DebugView++Lib/Colors.h"
#include "DebugView++Lib/ProcessInfo.h"

#pragma comment(lib, "psapi.lib")

namespace fusion {
namespace debugviewpp {

InternalProcessProperties::InternalProcessProperties() :
    pid(0),
    color(0)
{
}

InternalProcessProperties::InternalProcessProperties(DWORD pid, const std::wstring& name, COLORREF color) :
    pid(pid),
    name(name),
    color(color)
{
}

ProcessProperties::ProcessProperties(const InternalProcessProperties& iprops) :
    uid(0),
    pid(iprops.pid),
    name(iprops.name),
    color(iprops.color)
{
}

ProcessInfo::ProcessInfo() :
    m_unqiueId(0)
{
}

void ProcessInfo::Clear()
{
    m_unqiueId = 0;
    m_processProperties.clear();
}

size_t ProcessInfo::GetPrivateBytes()
{
    PROCESS_MEMORY_COUNTERS_EX memoryCounters;
    GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memoryCounters), sizeof(memoryCounters));
    return memoryCounters.PrivateUsage;
}

std::wstring ProcessInfo::GetProcessName(HANDLE handle)
{
    std::array<wchar_t, MAX_PATH> buf;
    auto rc = GetProcessImageFileName(handle, buf.data(), static_cast<DWORD>(buf.size()));
    if (rc == 0)
        return L"";

    // todo: make function? what does this do? (jan)
    const wchar_t* name = buf.data();
    for (auto it = buf.data(); *it; ++it)
    {
        if (*it == '\\')
            name = it + 1;
    }
    return name;
}

std::wstring ProcessInfo::GetStartTime(HANDLE handle)
{
    FILETIME creation = {};
    FILETIME exit = {};
    FILETIME kernel = {};
    FILETIME user = {};
    GetProcessTimes(handle, &creation, &exit, &kernel, &user);
    return WStr(GetTimeText(creation)).str();
}

std::wstring ProcessInfo::GetProcessNameByPid(DWORD processId)
{
    Win32::Handle hProcess(::OpenProcess(PROCESS_QUERY_INFORMATION, false, processId));
    if (hProcess)
        return GetProcessName(hProcess.get());

    return L"";
}

DWORD ProcessInfo::GetUid(DWORD processId, const std::wstring& processName)
{
    for (auto& item : m_processProperties)
    {
        if (item.second.pid == processId && item.second.name == processName)
            return item.first;
    }

    DWORD index = m_unqiueId;
    ++m_unqiueId;

    m_processProperties[index] = InternalProcessProperties(processId, processName, GetRandomProcessColor());
    return index;
}

ProcessProperties ProcessInfo::GetProcessProperties(DWORD processId, const std::wstring& processName)
{
    auto uid = GetUid(processId, processName);
    ProcessProperties props(m_processProperties[uid]);
    props.uid = uid;
    return props;
}

ProcessProperties ProcessInfo::GetProcessProperties(DWORD uid) const
{
    auto it = m_processProperties.find(uid);
    assert(it != m_processProperties.end());
    if (it == m_processProperties.end())
        return ProcessProperties(InternalProcessProperties());

    return ProcessProperties(it->second);
}

} // namespace debugviewpp
} // namespace fusion
