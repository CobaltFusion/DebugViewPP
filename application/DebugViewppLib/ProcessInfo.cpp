// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "DebugViewppLib/ProcessInfo.h"
#include "CobaltFusion/Str.h"
#include "Win32/Win32Lib.h"
#include "DebugViewppLib/Conversions.h"
#include "DebugViewppLib/Colors.h"

#include <cassert>
#include <array>
#include <vector>

#include "windows.h"
#include <psapi.h>

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
    {
        return L"";
    }

    // todo: make function? what does this do? (jan)
    const wchar_t* name = buf.data();
    for (auto it = buf.data(); *it != 0u; ++it)
    {
        if (*it == '\\')
        {
            name = it + 1;
        }
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

std::wstring ProcessInfo::GetProcessCommandLine(HANDLE handle)
{
    // NtQueryInformationProcess(ProcessCommandLineInformation) is available since Windows 8.1, so resolve it at runtime.
    using NtQueryInformationProcessFn = LONG(NTAPI*)(HANDLE, ULONG, PVOID, ULONG, PULONG);
    static const auto ntQueryInformationProcess = reinterpret_cast<NtQueryInformationProcessFn>(::GetProcAddress(::GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationProcess"));
    if (ntQueryInformationProcess == nullptr)
    {
        return L"";
    }

    constexpr ULONG ProcessCommandLineInformation = 60;
    ULONG size = 0;
    ntQueryInformationProcess(handle, ProcessCommandLineInformation, nullptr, 0, &size); // fails with the required size
    if (size == 0 || size > 1024 * 1024)
    {
        return L"";
    }

    // the result is a UNICODE_STRING followed by the characters it points to
    struct UnicodeString
    {
        USHORT length; // in bytes
        USHORT maximumLength;
        PWSTR buffer;
    };

    std::vector<unsigned char> buffer(size);
    if (ntQueryInformationProcess(handle, ProcessCommandLineInformation, buffer.data(), size, &size) < 0)
    {
        return L"";
    }

    const auto* commandLine = reinterpret_cast<const UnicodeString*>(buffer.data());
    if (commandLine->buffer == nullptr)
    {
        return L"";
    }
    return std::wstring(commandLine->buffer, commandLine->length / sizeof(wchar_t));
}

std::wstring ProcessInfo::GetProcessNameByPid(DWORD processId)
{
    Win32::Handle hProcess(::OpenProcess(PROCESS_QUERY_INFORMATION, 0, processId));
    if (hProcess)
    {
        return GetProcessName(hProcess.get());
    }

    return L"";
}

DWORD ProcessInfo::GetUid(DWORD processId, const std::wstring& processName)
{
    for (auto& item : m_processProperties)
    {
        if (item.second.pid == processId && item.second.name == processName)
        {
            return item.first;
        }
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
    {
        return ProcessProperties(InternalProcessProperties());
    }

    return ProcessProperties(it->second);
}

bool ProcessCommandLines::Has(DWORD pid) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_commandLines.find(pid) != m_commandLines.end();
}

void ProcessCommandLines::Set(DWORD pid, const std::wstring& commandLine)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_commandLines[pid] = commandLine;
}

std::wstring ProcessCommandLines::Take(DWORD pid)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_commandLines.find(pid);
    if (it == m_commandLines.end())
    {
        return L"";
    }
    auto commandLine = std::move(it->second);
    m_commandLines.erase(it);
    return commandLine;
}

} // namespace debugviewpp
} // namespace fusion
