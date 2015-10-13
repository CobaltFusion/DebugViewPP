// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "stdafx.h"
#include "Win32/Win32Lib.h"
#include "DebugView++Lib/ProcessMonitor.h"

namespace fusion {
namespace debugviewpp {

ProcessMonitor::ProcessInfo::ProcessInfo(DWORD pid, HANDLE handle) :
	pid(pid), handle(handle)
{
}

ProcessMonitor::ProcessMonitor() :
	m_end(false),
	m_event(Win32::CreateEvent(nullptr, false, false, nullptr)),
	m_thread([this] { Run(); })
{
}

ProcessMonitor::~ProcessMonitor()
{
	m_q.Push([this] { m_end = true; });
	Win32::SetEvent(m_event);
	m_thread.join();
}

void ProcessMonitor::Add(DWORD pid, HANDLE handle)
{
	m_q.Push([this, pid, handle]
	{
		m_processes.push_back(ProcessMonitor::ProcessInfo(pid, handle));
	});
	Win32::SetEvent(m_event);
}

boost::signals2::connection ProcessMonitor::ConnectProcessEnded(ProcessEnded::slot_type slot)
{
	return m_processEnded.connect(slot);
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
		auto result = Win32::WaitForAnyObject(handles.data(), handles.data() + count + 1, timeout);
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
			m_processEnded(m_processes[i].pid, m_processes[i].handle);
			m_processes[i] = m_processes.back();
			m_processes.resize(processCount - 1);
		}
	}
}

} // namespace debugviewpp 
} // namespace fusion
