// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <unordered_map>
#include "Win32Lib/Win32Lib.h"

namespace fusion {
namespace debugviewpp {

typedef std::unordered_map<DWORD, Handle> PidMap;

class ProcessHandleCache
{
public:
	~ProcessHandleCache();

	void Add(DWORD pid, Handle handle);
	PidMap CleanupMap();

private:
	PidMap m_cache;
};

} // namespace debugviewpp 
} // namespace fusion
