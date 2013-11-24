//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://boosttestui.wordpress.com/ for the boosttestui home page.

#include "stdafx.h"
#include <boost/utility.hpp>
#include <boost/signals2.hpp>
#include <boost/thread.hpp>
#include "Utilities.h"

namespace gj {

struct Line
{
	TickType ustime;
	DWORD pid;
	std::string message;
};

typedef std::vector<Line> LinesList;

class DBWinReader : boost::noncopyable
{
public:
	typedef boost::signals2::signal<void (DWORD, const char*)> OnMessage;

	explicit DBWinReader(bool global);
	~DBWinReader();

	void Abort();

	LinesList GetLines();

private:
	void Run();

	bool m_end;
	CHandle m_hBuffer;
	CHandle m_dbWinBufferReady;
	CHandle m_dbWinDataReady;
	boost::thread m_thread;

	LinesList m_lines;
	boost::mutex m_linesMutex;
	AccurateTime m_accurateTime;
};

} // namespace gj
