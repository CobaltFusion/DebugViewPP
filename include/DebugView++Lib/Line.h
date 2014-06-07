// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include "Win32Lib/Win32Lib.h"

namespace fusion {
namespace debugviewpp {

class LogSource;

struct InputLine
{
	InputLine(double time, FILETIME systemTime, HANDLE handle, const std::string& message, LogSource* logsource);
	InputLine(double time = 0.0, FILETIME systemTime = FILETIME(), DWORD pid = 0, const std::string& processName = "", const std::string& message = "", LogSource* logsource = nullptr);

	double time;
	FILETIME systemTime;
	HANDLE handle;
	DWORD pid;
	std::string processName;
	std::string message;
	LogSource* logsource;
};

typedef std::vector<InputLine> InputLines;

} // namespace debugviewpp 
} // namespace fusion
