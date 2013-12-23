// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "ProcessInfo.h"
#include "Win32Lib.h"
#include "Utilities.h"
#include <array>
#pragma comment(lib, "psapi.lib")

namespace fusion {

ProcessInfo::ProcessInfo()
{
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

std::wstring ProcessInfo::GetProcessName(DWORD processId)
try
{
	Handle hProcess(OpenProcess(PROCESS_QUERY_INFORMATION, false, processId));
	if (hProcess)
	{
		return GetProcessName(hProcess.get());
	}
	return L"";
}
catch (Win32Error& e)
{
	return wstringbuilder() << e.what();
}

DWORD ProcessInfo::GetUid(DWORD processId, const std::string& processName)
{
	static DWORD static_uid = 0;
	for (auto i = m_processProperties.begin(); i != m_processProperties.end(); i++)
	{
		if (i->second.pid == processId && i->second.name == processName)
		{
			return i->first;
		}
	}

	DWORD index = static_uid;
	static_uid++;

	m_processProperties[index] = InternalProcessProperties(processId, processName);
	return index;
}

ProcessProperties ProcessInfo::GetProcessProperties(DWORD processId, const std::string& processName)
{
	auto uid = GetUid(processId, processName);
	return ProcessProperties(m_processProperties[uid]);
}

ProcessProperties ProcessInfo::GetProcessProperties(DWORD processId, HANDLE handle)
{
	auto uid = GetUid(processId, Str(GetProcessName(handle)));
	return ProcessProperties(m_processProperties[uid]);
}

ProcessProperties ProcessInfo::GetProcessProperties(DWORD uid) const
{
	auto processMap = const_cast<ProcessMap&>(m_processProperties);		// AUW! todo: Fix!
	auto props = processMap[uid];
	//auto props = m_processProperties[uid];
	return ProcessProperties(props);
}

} // namespace fusion
