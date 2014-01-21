// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <boost/utility.hpp>
#include <boost/thread.hpp>

#include "Win32Lib.h"
#include "LogSource.h"
#include "DBWinBuffer.h"
#include "Utilities.h"
#include "ProcessHandleCache.h"

namespace fusion {
namespace debugviewpp {

struct InternalLine
{
	double time;
	FILETIME systemTime;
	DWORD pid;
	std::string message;
	HANDLE handle;
};

typedef std::vector<InternalLine> InternalLines;

class DBWinReader : public LogSource
{
public:
	explicit DBWinReader(bool global);
	~DBWinReader();

	virtual bool AtEnd() const;
	virtual HANDLE GetHandle() const;
	virtual Line GetLine();
	virtual Lines GetLines();

	void Abort();

	bool AutoNewLine() const;
	void AutoNewLine(bool value);

private:
	void Run();
	void Add(DWORD pid, const char* text, HANDLE handle);
	void AddLine(const InternalLine& InternalLine);
	Lines ProcessLines(const InternalLines& lines);
	Lines ProcessLine(const Line& internalLine);
	Lines CheckHandleCache();

	InternalLines m_lines;
	InternalLines m_backBuffer;
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
};

} // namespace debugviewpp 
} // namespace fusion
