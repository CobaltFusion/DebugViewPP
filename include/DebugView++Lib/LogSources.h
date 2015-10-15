// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <boost/signals2.hpp>

#pragma warning(push, 1)
#include <boost/thread.hpp>
#pragma warning(pop)
#include "Win32/Win32Lib.h"
#include "DebugView++Lib/LogSource.h"
#include "DebugView++Lib/LineBuffer.h"
#include "DebugView++Lib/VectorLineBuffer.h"
#include "CobaltFusion/CircularBuffer.h"
#include "CobaltFusion/GuiExecutor.h"
#include "DebugView++Lib/NewlineFilter.h"
#include "DebugView++Lib/ProcessMonitor.h"

#pragma comment(lib, "DebugView++Lib.lib")

namespace fusion {
namespace debugviewpp {

class DBWinReader;
class ProcessReader;
class FileReader;
class DBLogReader;
class BinaryFileReader;
class PipeReader;
class TestSource;
class Loopback;
class DbgviewReader;
class SocketReader;

typedef std::vector<HANDLE> LogSourceHandles;

class LogSources
{
public:
	typedef boost::signals2::signal<bool ()> Update;

	explicit LogSources(bool startListening = false);
	~LogSources();

	void SetAutoNewLine(bool value);
	bool GetAutoNewLine() const;

	void Reset();
	void Listen();
	void ListenUntilUpdateEvent();
	void Abort();
	Lines GetLines();
	void Remove(std::shared_ptr<LogSource> logsource);
	void InternalRemove(std::shared_ptr<LogSource> logsource);
	std::vector<std::shared_ptr<LogSource>> GetSources();

	std::shared_ptr<DBWinReader> AddDBWinReader(bool global);
	std::shared_ptr<ProcessReader> AddProcessReader(const std::wstring& pathName, const std::wstring& args);
	std::shared_ptr<FileReader> AddFileReader(const std::wstring& filename);
	std::shared_ptr<BinaryFileReader> AddBinaryFileReader(const std::wstring& filename);
	std::shared_ptr<DBLogReader> AddDBLogReader(const std::wstring& filename);
	std::shared_ptr<DbgviewReader> AddDbgviewReader(const std::string& hostname);
	std::shared_ptr<SocketReader> AddUDPReader(int port);
	std::shared_ptr<PipeReader> AddPipeReader(DWORD pid, HANDLE hPipe);
	std::shared_ptr<TestSource> AddTestSource();		// for unittesting
	void AddMessage(const std::string& message);
	boost::signals2::connection SubscribeToUpdate(Update::slot_type slot);

private:
	void UpdateSettings(std::shared_ptr<LogSource> source);
	void Add(std::shared_ptr<LogSource> source);
	void OnProcessEnded(DWORD pid, HANDLE handle);
	void OnUpdate();
	void DelayedUpdate();

	bool m_autoNewLine;
	boost::mutex m_mutex;
	std::vector<std::shared_ptr<LogSource>> m_sources;
	Win32::Handle m_updateEvent;
	bool m_end;
	VectorLineBuffer m_linebuffer;
	PidMap m_pidMap;
	ProcessMonitor m_processMonitor;
	NewlineFilter m_newlineFilter;
	std::shared_ptr<Loopback> m_loopback;
	Timer m_timer;

	GuiExecutor m_guiExecutor;
	bool m_dirty;
	Update m_update;

	// make sure this thread is last to initialize
	boost::thread m_listenThread;
};

} // namespace debugviewpp 
} // namespace fusion
