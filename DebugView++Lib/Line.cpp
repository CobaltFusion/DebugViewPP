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

InputLine::InputLine(double time, FILETIME systemTime, HANDLE handle, const std::string& message, LogSource* logsource) :
	time(time),
	systemTime(systemTime),
	handle(handle),
	message(message),
	logsource(logsource)
{
}

InputLine::InputLine(double time, FILETIME systemTime, DWORD pid, const std::string& processName, const std::string& message, LogSource* logsource) :
	time(time),
	systemTime(systemTime),
	handle(INVALID_HANDLE_VALUE),
	pid(pid),
	processName(processName),
	message(message),
	logsource(logsource)
{
}
	

} // namespace debugviewpp 
} // namespace fusion
