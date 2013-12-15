// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include "psapi.h"

namespace fusion {

class ProcessInfo
{
public:
	ProcessInfo();
	void Refresh();
	size_t GetPrivateBytes() const;
	static std::wstring GetProcessName(DWORD processId);

private:
	DWORD m_pid;
	HANDLE m_handle;
	PROCESS_MEMORY_COUNTERS_EX m_memoryCounters;
};

} // namespace fusion
