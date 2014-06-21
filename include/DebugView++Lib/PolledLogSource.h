// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <boost/utility.hpp>
#include <boost/thread.hpp>
#include <boost/signals2.hpp>

#include "LogSource.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

struct LoopLine
{
	LoopLine(DWORD pid, const std::string& processName, const std::string& message, LogSource* logsource);
	DWORD pid;
	std::string processName;
	std::string message;
	LogSource* logsource;
};

class PolledLogSource : public LogSource
{
public:
	explicit PolledLogSource(ILineBuffer& linebuffer, long pollFrequency);
	~PolledLogSource();
	
	virtual HANDLE GetHandle() const;
	virtual void Notify();

private:
	void Abort();
	void Loop();
	void AddMessage(DWORD pid, const char* processName, const char* message);
	void Signal();
	virtual bool Poll() = 0;

	std::vector<LoopLine> m_lines;
	Handle m_handle;
	boost::mutex m_mutex;
	long m_microsecondInterval;
	boost::thread m_thread;
};

} // namespace debugviewpp 
} // namespace fusion
