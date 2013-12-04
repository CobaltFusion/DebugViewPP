#pragma once

#include <string>
#include "psapi.h"

namespace gj {

class ProcessInfo
{
public:
	ProcessInfo();
	void Refresh();
	SIZE_T GetPrivateBytes();
	static std::wstring GetProcessName(DWORD processId);

private:
	DWORD m_pid;
	HANDLE m_handle;
	PROCESS_MEMORY_COUNTERS_EX m_memoryCounters;
};

} // namespace gj
