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

ProcessInfo::ProcessInfo() :
	m_pid(GetCurrentProcessId()),
	m_handle(GetCurrentProcess())
{
	Refresh();
}

void ProcessInfo::Refresh()
{
	GetProcessMemoryInfo(m_handle, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&m_memoryCounters), sizeof(m_memoryCounters));
}

size_t ProcessInfo::GetPrivateBytes() const
{
	return m_memoryCounters.PrivateUsage;
}

std::wstring ProcessInfo::GetProcessName(DWORD processId)
{
	std::array<wchar_t, MAX_PATH> buf;
	try
	{
		Handle hProcess(OpenProcess(PROCESS_QUERY_INFORMATION, false, processId));
		DWORD rc = GetProcessImageFileName(hProcess.get(), buf.data(), buf.size());
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

} // namespace fusion
