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

Pids ProcessHandleCache::Cleanup()
{
	Pids removePids;
	for (auto i = m_cache.begin(); i != m_cache.end(); ++i)
	{
		DWORD exitcode = 0;
		BOOL result = GetExitCodeProcess(i->second.get(), &exitcode);
		if (result == FALSE || exitcode != STILL_ACTIVE)
		{
			DWORD pid = i->first;
			removePids.push_back(pid);
		}
	}

	for (auto i = removePids.begin(); i != removePids.end(); ++i)
		m_cache.erase(*i);
	return removePids;
}

PIDMap ProcessHandleCache::CleanupMap()
{
	PIDMap removePids;
	for (auto i = m_cache.begin(); i != m_cache.end(); ++i)
	{
		DWORD exitcode = 0;
		BOOL result = GetExitCodeProcess(i->second.get(), &exitcode);
		if (result == FALSE || exitcode != STILL_ACTIVE)
		{
			DWORD pid = i->first;
			removePids[pid] = std::move(i->second);
		}
	}

	for (auto i = removePids.begin(); i != removePids.end(); ++i)
		m_cache.erase(i->first);
	return removePids;
}

} // namespace debugviewpp 
} // namespace fusion
