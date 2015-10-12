// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "stdafx.h"
#include "Win32Lib/Win32Lib.h"
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

ProcessMonitor::ProcessInfo::ProcessInfo(DWORD pid, HANDLE handle) :
	pid(pid), handle(handle)
{
}

	ProcessMonitor::ProcessMonitor() :
	m_end(false),
	m_event(CreateEvent(nullptr, false, false, nullptr)),
	m_thread([this] { Run(); })
{
}

ProcessMonitor::~ProcessMonitor()
{
	m_q.Push([this] { m_end = true; });
	SetEvent(m_event.get());
	m_thread.join();
}

void ProcessMonitor::Add(DWORD pid, HANDLE handle)
{
	m_q.Push([this, pid, handle]
	{
		m_processes.push_back(ProcessMonitor::ProcessInfo(pid, handle));
	});
	SetEvent(m_event.get());
}

boost::signals2::connection ProcessMonitor::ConnectProcessEnd(ProcessEnd::slot_type slot)
{
	return m_processEnd.connect(slot);
}

void ProcessMonitor::Run()
{
	int offset = 0;
	while (!m_end)
	{
		std::array<HANDLE, MAXIMUM_WAIT_OBJECTS> handles;
		handles[0] = m_event.get();
		int processCount = m_processes.size();
		int count = std::min(processCount, MAXIMUM_WAIT_OBJECTS - 1);
		for (int i = 0; i < count; ++i)
			handles[i + 1] = m_processes[(offset + i) % processCount].handle;
		DWORD timeout = static_cast<size_t>(count) < m_processes.size() ? 1000 : INFINITE;
		auto result = WaitForAnyObject(handles.data(), handles.data() + count + 1, timeout);
		if (!result.signaled)
		{
			offset = (offset + count) % processCount;
		}
		else if (result.index == 0)
		{
			while (!m_q.Empty())
				m_q.Pop()();
		}
		else
		{
			int i = (offset + result.index - 1) % processCount;
			m_processEnd(m_processes[i].pid, m_processes[i].handle);
			m_processes[i] = m_processes.back();
			m_processes.resize(processCount - 1);
		}
	}
}

} // namespace debugviewpp 
} // namespace fusion
