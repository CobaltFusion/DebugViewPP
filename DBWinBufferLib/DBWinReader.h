// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <boost/utility.hpp>
#include <boost/thread.hpp>

#include "LogSource.h"
#include "DBWinBuffer.h"
#include "ProcessHandleCache.h"
#include <boost/signals2.hpp>

namespace fusion {
namespace debugviewpp {

struct DBWinMessage
{
	double time;
	FILETIME systemTime;
	DWORD pid;
	std::string message;
	HANDLE handle;
};

typedef std::vector<DBWinMessage> DBWinMessages;

typedef void OnDBWinMessage(double time, FILETIME systemTime, DWORD processId, HANDLE processHandle, const char* message);

class DBWinReader : public LogSource
{
public:
	explicit DBWinReader(bool global, bool startlistening = true);
	~DBWinReader();

	virtual bool AtEnd() const;
	virtual HANDLE GetHandle() const;
	virtual void Notify();
	virtual Lines GetLines();

	void Abort();

	boost::signals2::connection Connect(const std::function<OnDBWinMessage>&);

	bool AutoNewLine() const;
	void AutoNewLine(bool value);

private:
	void Run();
	void Add(DWORD pid, const char* text, HANDLE handle);
	void AddLine(const DBWinMessage& DBWinMessage);
	Lines ProcessLines(const DBWinMessages& lines);
	Lines ProcessLine(const Line& DBWinMessage);
	Lines CheckHandleCache();

	DBWinMessages m_lines;
	DBWinMessages m_backBuffer;
	mutable boost::mutex m_linesMutex;
	Timer m_timer;

	bool m_autoNewLine;
	bool m_end;
	Handle m_hBuffer;
	Handle m_dbWinBufferReady;
	Handle m_dbWinDataReady;
	
	double m_handleCacheTime;
	std::map<DWORD, std::string> m_lineBuffers;

	// make sure the thread is last to initialize
	MappedViewOfFile m_mappedViewOfFile;
	const DbWinBuffer* m_dbWinBuffer;

	ProcessHandleCache m_handleCache;
	boost::thread m_thread;
	boost::signals2::signal<OnDBWinMessage> m_onDBWinMessage;
};

} // namespace debugviewpp 
} // namespace fusion
