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
#include "DebugView++Lib/VectorLineBuffer.h"
#include "CobaltFusion/CircularBuffer.h"
#include "DebugView++Lib/NewlineFilter.h"

#pragma comment(lib, "DebugView++Lib.lib")

namespace fusion {
namespace debugviewpp {

class DBWinReader;
class ProcessReader;
class FileReader;
class DBLogReader;
class PipeReader;
class TestSource;
class Loopback;
class DbgviewReader;

typedef std::vector<HANDLE> LogSourceHandles;

class LogSources
{
public:
	LogSources(bool startListening = false);
	~LogSources();

	void FlushTrash();

	void SetAutoNewLine(bool value);
	bool GetAutoNewLine() const;

	void Reset();
	void Listen();
	void Abort();
	Lines GetLines();
	void Remove(std::shared_ptr<LogSource> logsource);
	void InternalRemove(std::shared_ptr<LogSource> logsource);
	std::vector<std::shared_ptr<LogSource>> GetSources();

	std::shared_ptr<DBWinReader> AddDBWinReader(bool global);
	std::shared_ptr<ProcessReader> AddProcessReader(const std::wstring& pathName, const std::wstring& args);
	std::shared_ptr<FileReader> AddFileReader(const std::wstring& filename);
	std::shared_ptr<DBLogReader> AddDBLogReader(const std::wstring& filename);
	std::shared_ptr<DbgviewReader> AddDbgviewReader(const std::string& hostname);
	std::shared_ptr<PipeReader> AddPipeReader(DWORD pid, HANDLE hPipe);
	std::shared_ptr<TestSource> AddTestSource();		// for unittesting
	void AddMessage(const std::string& message);
private:
	void Add(std::shared_ptr<LogSource> source);
	void CheckForTerminatedProcesses();

	bool m_autoNewLine;
	boost::mutex m_mutex;
	std::vector<std::shared_ptr<LogSource>> m_sources;
	std::vector<std::shared_ptr<LogSource>> m_trash;
	Handle m_updateEvent;
	bool m_end;
	VectorLineBuffer m_linebuffer;				// Replace with LineBuffer (which is circular) once it is finished, currently buggy...
	ProcessHandleCache m_handleCache;
	NewlineFilter m_newlineFilter;
	std::shared_ptr<Loopback> m_loopback;
	Timer m_timer;
	double m_handleCacheTime;

	// make sure the thread is last to initialize
	boost::thread m_thread;
};

} // namespace debugviewpp 
} // namespace fusion
