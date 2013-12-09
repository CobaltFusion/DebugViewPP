// (C) Copyright Gert-Jan de Vos 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// See http://boosttestui.wordpress.com/ for the boosttestui home page.

#pragma once

#include <string>
#include "psapi.h"

namespace gj {

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

} // namespace gj
