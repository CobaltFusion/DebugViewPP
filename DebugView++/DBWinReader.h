//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://boosttestui.wordpress.com/ for the boosttestui home page.

#include "stdafx.h"
#include <boost/utility.hpp>
#include <boost/signals2.hpp>
#include <boost/thread.hpp>

namespace gj {

class DBWinReader : boost::noncopyable
{
public:
	typedef boost::signals2::signal<void (DWORD, const char*)> OnMessage;

	explicit DBWinReader(bool global);
	~DBWinReader();

	boost::signals2::connection ConnectOnMessage(OnMessage::slot_type slot);
	void Abort();

private:
	void Run();

	bool m_end;
	CHandle bufferMutex;
	CHandle hBuffer;
	CHandle dbWinBufferReady;
	CHandle dbWinDataReady;
	OnMessage m_onMessage;
	boost::thread m_thread;
};

} // namespace gj
