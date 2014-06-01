// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Win32Lib/Win32Lib.h"
#include "DebugView++Lib/Line.h"

namespace fusion {
namespace debugviewpp {

Line::Line(double time, FILETIME systemTime, DWORD pid, const std::string& processName, const std::string& message) :
	time(time),
	systemTime(systemTime),
	pid(pid),
	processName(processName),
	message(message)
{
}

} // namespace debugviewpp 
} // namespace fusion
