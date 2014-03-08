// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <array>
#include "Win32Lib/Win32Lib.h"
#include "DBWinBufferLib/Colors.h"
#include "DBWinBufferLib/ProcessInfo.h"

#pragma comment(lib, "psapi.lib")

namespace fusion {
namespace debugviewpp {

InternalProcessProperties::InternalProcessProperties() :
	pid(0)
{
}

InternalProcessProperties::InternalProcessProperties(DWORD pid, const std::string& name, COLORREF color) :
	pid(pid), name(name), color(color)
{
}

ProcessProperties::ProcessProperties(const InternalProcessProperties& iprops) :
	uid(0), pid(iprops.pid), name(iprops.name), color(iprops.color)
{
}

ProcessInfo::ProcessInfo() : m_unqiueId(0)
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
	DWORD rc = GetProcessImageFileName(handle, buf.data(), buf.size());
	if (rc == 0)
		return L"";

	const wchar_t* name = buf.data();
	for (auto it = buf.data(); *it; ++it)
	{
		if (*it == '\\')
			name = it + 1;
	}
	return name;
}

std::wstring ProcessInfo::GetProcessNameByPid(DWORD processId)
{
	Handle hProcess(::OpenProcess(PROCESS_QUERY_INFORMATION, false, processId));
	if (hProcess)
		return GetProcessName(hProcess.get());

	return L"";
}

DWORD ProcessInfo::GetUid(DWORD processId, const std::string& processName)
{
	for (auto i = m_processProperties.begin(); i != m_processProperties.end(); ++i)
	{
		if (i->second.pid == processId && i->second.name == processName)
			return i->first;
	}

	DWORD index = m_unqiueId;
	++m_unqiueId;

	m_processProperties[index] = InternalProcessProperties(processId, processName, GetRandomProcessColor());
	return index;
}

ProcessProperties ProcessInfo::GetProcessProperties(DWORD processId, const std::string& processName)
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
