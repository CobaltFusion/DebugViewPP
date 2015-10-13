// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <unordered_map>
#include <boost/signals2.hpp>
#include "Win32/Win32Lib.h"
#include "CobaltFusion/SynchronizedQueue.h"
#include <boost/thread.hpp>

namespace fusion {
namespace debugviewpp {

typedef std::unordered_map<DWORD, Win32::Handle> PidMap;

class ProcessMonitor
{
public:
	typedef boost::signals2::signal<void (DWORD, HANDLE)> ProcessEnded;

	ProcessMonitor();
	~ProcessMonitor();

	void Add(DWORD pid, HANDLE handle);
	boost::signals2::connection ConnectProcessEnded(ProcessEnded::slot_type slot);

private:
	struct ProcessInfo
	{
		ProcessInfo(DWORD pid = 0, HANDLE handle = nullptr);

		DWORD pid;
		HANDLE handle;
	};

	void Run();

	bool m_end;
	Win32::Handle m_event;
	ProcessEnded m_processEnded;
	std::vector<ProcessInfo> m_processes;
	SynchronizedQueue<std::function<void ()>> m_q;
	boost::thread m_thread;
};

} // namespace debugviewpp 
} // namespace fusion
