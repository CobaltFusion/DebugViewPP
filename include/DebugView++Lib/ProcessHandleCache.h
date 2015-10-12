// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <unordered_map>
#include <boost/signals2.hpp>
#include "Win32Lib/Win32Lib.h"
#include "CobaltFusion/SynchronizedQueue.h"

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

class ProcessMonitor
{
public:
	typedef boost::signals2::signal<void (DWORD, HANDLE)> ProcessEnd;

	ProcessMonitor();
	~ProcessMonitor();

	void Add(DWORD pid, HANDLE handle);
	boost::signals2::connection ConnectProcessEnd(ProcessEnd::slot_type slot);

private:
	struct ProcessInfo
	{
		ProcessInfo(DWORD pid = 0, HANDLE handle = nullptr);

		DWORD pid;
		HANDLE handle;
	};

	void Run();

	bool m_end;
	Handle m_event;
	ProcessEnd m_processEnd;
	std::vector<ProcessInfo> m_processes;
	SynchronizedQueue<std::function<void ()>> m_q;
	boost::thread m_thread;
};

} // namespace debugviewpp 
} // namespace fusion
