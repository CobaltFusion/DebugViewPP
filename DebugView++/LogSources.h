// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "LogSource.h"
#include "Win32Lib.h"
#include "CircularBuffer.h"

namespace fusion {
namespace debugviewpp {

struct LogSourceInfo {
	LogSourceInfo(HANDLE handle, LogSource& logsource);		
	HANDLE handle;
	LogSource& logsource;
};

typedef std::vector<LogSourceInfo> LogSourcesVector;
typedef std::vector<HANDLE> LogSourcesHandles;

class LogSources
{
public:
	LogSources();
	~LogSources();

	void Add(std::unique_ptr<LogSource> source);
	void Run();
	void Abort();
	Lines GetLines();

private:
	LogSourcesHandles GetWaitHandles();
	void Process(int index);

	boost::mutex m_mutex;
	std::vector<std::unique_ptr<LogSource>> m_sources;
	Handle m_updateEvent;
	bool m_end;
	bool m_sourcesDirty;
	CircularBuffer m_circularBuffer;
	LogSourcesHandles m_waitHandles;
	// make sure the thread is last to initialize
	boost::thread m_thread;
};

} // namespace debugviewpp 
} // namespace fusion
