// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "stdafx.h"
#include "DebugView++Lib/ProcessHandleCache.h"

namespace fusion {
namespace debugviewpp {

ProcessHandleCache::~ProcessHandleCache()
{
}

void ProcessHandleCache::Add(DWORD pid, Handle handle)
{
	if (m_cache.find(pid) == m_cache.end())
		m_cache[pid] = std::move(handle);
}

PidMap ProcessHandleCache::CleanupMap()
{
	PidMap removePids;
	for (auto it = m_cache.begin(); it != m_cache.end(); ++it)
	{
		DWORD exitcode = 0;
		BOOL result = GetExitCodeProcess(it->second.get(), &exitcode);
		if (result == FALSE || exitcode != STILL_ACTIVE)
		{
			DWORD pid = it->first;
			removePids[pid] = std::move(it->second);
		}
	}

	for (auto it = removePids.begin(); it != removePids.end(); ++it)
		m_cache.erase(it->first);
	return removePids;
}

} // namespace debugviewpp 
} // namespace fusion
