// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <boost/utility.hpp>
#include <boost/signals2.hpp>
#include <boost/thread.hpp>

#include "Win32Lib.h"
#include "Utilities.h"

namespace fusion {

struct Line
{
	double time;
	FILETIME systemTime;
	DWORD pid;
	std::string message;
};

typedef std::vector<Line> Lines;

class DBWinReader : boost::noncopyable
{
public:
	explicit DBWinReader(bool global);
	~DBWinReader();

	bool AutoNewLine() const;
	void AutoNewLine(bool value);

	void Abort();
	const Lines& GetLines();

private:
	void Run();
	void Add(DWORD pid, const char* text);
	void AddLine(const Line& line);

	Lines m_lines;
	Lines m_linesBackingBuffer;
	Line m_lineBuffer;
	mutable boost::mutex m_linesMutex;
	Timer m_timer;

	bool m_autoNewLine;
	bool m_end;
	Handle m_hBuffer;
	Handle m_dbWinBufferReady;
	Handle m_dbWinDataReady;
	boost::thread m_thread;
};

} // namespace fusion
