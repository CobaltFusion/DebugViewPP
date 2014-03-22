// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <boost/thread.hpp>
#include "Win32Lib/Win32Lib.h"
#include "DebugView++Lib/LogSource.h"
#include "DebugView++Lib/LineBuffer.h"

#pragma comment(lib, "DebugView++Lib.lib")

namespace fusion {
namespace debugviewpp {

class DBWinReader;
class ProcessReader;
class FileReader;
class DBLogReader;
class PipeReader;

struct LogSourceInfo
{
	LogSourceInfo(HANDLE handle, LogSource& logsource);		
	HANDLE handle;
	LogSource& logsource;
};

typedef std::vector<LogSourceInfo> LogSourcesVector;
typedef std::vector<HANDLE> LogSourcesHandles;

class LogSources
{
public:
	LogSources(bool startListening = false);
	~LogSources();

	void Remove(std::shared_ptr<LogSource> logsource);

	void Listen();
	void Abort();
	Lines GetLines();

	std::vector<std::shared_ptr<LogSource>> Get();

	std::shared_ptr<DBWinReader> AddDBWinReader(bool global);
	std::shared_ptr<ProcessReader> AddProcessReader(const std::wstring& pathName, const std::wstring& args);
	std::shared_ptr<FileReader> AddFileReader(const std::wstring& filename);
	std::shared_ptr<DBLogReader> AddDBLogReader(const std::wstring& filename);
	std::shared_ptr<PipeReader> AddPipeReader(DWORD pid, HANDLE hPipe);

private:
	LogSourcesHandles GetWaitHandles();
	void Process(int index);
	void Add(std::shared_ptr<LogSource> source);

	boost::mutex m_mutex;
	std::vector<std::shared_ptr<LogSource>> m_sources;
	Handle m_updateEvent;
	bool m_end;
	bool m_sourcesDirty;
	LogSourcesHandles m_waitHandles;
	// make sure the thread is last to initialize
	boost::thread m_thread;

	LineBuffer m_linebuffer;

};

} // namespace debugviewpp 
} // namespace fusion
