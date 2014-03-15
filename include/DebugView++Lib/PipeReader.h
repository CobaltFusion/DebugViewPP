// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <boost/utility.hpp>
#include "Win32Lib/utilities.h"
#include "DebugView++Lib/LogSource.h"

namespace fusion {
namespace debugviewpp {

class PipeReader :
	boost::noncopyable,
	public LogSource
{
public:
	PipeReader(HANDLE hPipe, DWORD pid, const std::string& processName);

	virtual bool AtEnd() const;
	virtual HANDLE GetHandle() const;
	virtual void Notify();
	virtual Lines GetLines();

private:
	Line MakeLine(const std::string& text) const;

	HANDLE m_hPipe;
	DWORD m_pid;
	std::string m_process;
	Timer m_timer;
	std::string m_buffer;
};

} // namespace debugviewpp 
} // namespace fusion
