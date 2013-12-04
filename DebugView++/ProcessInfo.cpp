//
//BOOL WINAPI GetProcessMemoryInfo(
//  __in   HANDLE Process,
//  __out  PPROCESS_MEMORY_COUNTERS ppsmemCounters,
//  __in   DWORD cb
//);
//
//// PROCESS_MEMORY_COUNTERS_EX

#include "stdafx.h"
#include "ProcessInfo.h"
#include <array>
#pragma comment(lib, "psapi.lib")

namespace gj {

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

SIZE_T ProcessInfo::GetPrivateBytes()
{
	return m_memoryCounters.PrivateUsage;
}

std::wstring ProcessInfo::GetProcessName(DWORD processId)
{
	CHandle hProcess(::OpenProcess(PROCESS_QUERY_INFORMATION, false, processId));
	if (!hProcess)
		return L"<terminated>";

	std::array<wchar_t, MAX_PATH> buf;
	DWORD rc = GetProcessImageFileName(hProcess, buf.data(), buf.size());
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

} // namespace gj
