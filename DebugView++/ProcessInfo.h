// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include "psapi.h"

namespace fusion {

struct ProcessProperties
{
	DWORD uid;			// unique id
	DWORD pid;			// system processId
	std::wstring name;
};

class ProcessInfo
{
	struct InternalProcessProperties
	{
		DWORD pid;			// system processId
		std::wstring name;
	};

public:
	ProcessInfo();
	static size_t GetPrivateBytes();
	static std::wstring GetProcessName(HANDLE handle);
	static std::wstring GetProcessName(DWORD processId);
	ProcessProperties GetProcessProperties(DWORD processId, HANDLE handle);

private:
	std::map<int, InternalProcessProperties> m_processProperties;
};

} // namespace fusion
