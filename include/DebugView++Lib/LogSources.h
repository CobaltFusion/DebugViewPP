// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <memory>
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

template<typename T>
void EraseElements(std::vector<std::unique_ptr<T>>& v, const std::vector<T*>& e)
{
	v.erase(
		std::remove_if( // Selectively remove elements in the second vector...
			v.begin(),
			v.end(),
			[&] (std::unique_ptr<T> const& p)
			{   // This predicate checks whether the element is contained
				// in the second vector of pointers to be removed...
				return std::find(
					e.cbegin(), 
					e.cend(), 
					p.get()
					) != e.end();
			}),
		v.end()
	);
}

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
	void Remove(LogSource* pLogSource);
	std::vector<LogSource*> GetSources() const;

	DBWinReader* AddDBWinReader(bool global);
	ProcessReader* AddProcessReader(const std::wstring& pathName, const std::wstring& args);
	FileReader* AddFileReader(const std::wstring& filename);
	BinaryFileReader* AddBinaryFileReader(const std::wstring& filename);
	DBLogReader* AddDBLogReader(const std::wstring& filename);
	DbgviewReader* AddDbgviewReader(const std::string& hostname);
	SocketReader* AddUDPReader(int port);
	PipeReader* AddPipeReader(DWORD pid, HANDLE hPipe);
	TestSource* AddTestSource();		// for unittesting
	void AddMessage(const std::string& message);
	boost::signals2::connection SubscribeToUpdate(Update::slot_type slot);

private:
	void InternalRemove(LogSource*);
	void UpdateSettings(const std::unique_ptr<LogSource>& pSource);
	void Add(std::unique_ptr<LogSource> pSource);
	void OnProcessEnded(DWORD pid, HANDLE handle);
	void OnUpdate();
	void DelayedUpdate();
	Loopback* CreateLoopback(Timer& timer, ILineBuffer& lineBuffer);

	bool m_autoNewLine;
	mutable boost::mutex m_mutex;
	std::vector<std::unique_ptr<LogSource>> m_sources;
	Win32::Handle m_updateEvent;
	bool m_end;
	VectorLineBuffer m_linebuffer;
	PidMap m_pidMap;
	ProcessMonitor m_processMonitor;
	NewlineFilter m_newlineFilter;
	Loopback* m_loopback;
	Timer m_timer;

	GuiExecutor m_guiExecutor;
	bool m_updatePending;
	Update m_update;

	// make sure this thread is last to initialize
	boost::thread m_listenThread;
};

} // namespace debugviewpp 
} // namespace fusion
