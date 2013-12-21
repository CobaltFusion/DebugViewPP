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
	try
	{
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
	catch (Win32Error& e)
	{
		return wstringbuilder() << e.what();
	}
}

std::wstring ProcessInfo::GetProcessName(DWORD processId)
{
	Handle hProcess(OpenProcess(PROCESS_QUERY_INFORMATION, false, processId));
	return GetProcessName(hProcess.get());
}

ProcessProperties ProcessInfo::GetProcessProperties(DWORD processId, HANDLE handle)
{
	static DWORD static_uid = 0;
	static_uid++;

	ProcessProperties props;
	props.uid = static_uid;
	props.pid = processId;
	props.name = GetProcessName(handle);
	return props;
}

} // namespace fusion
