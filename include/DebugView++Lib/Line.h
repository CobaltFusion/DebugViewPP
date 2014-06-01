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

struct InputLine		// remove existing Line struct and rename "InputLine" to "Line"
{
	InputLine(double time = 0.0, FILETIME systemTime = FILETIME(), HANDLE handle = nullptr, const std::string& message = "");

	double time;
	FILETIME systemTime;
	HANDLE handle;
	std::string message;
};

typedef std::vector<InputLine> InputLines;

struct Line
{
	Line(double time = 0.0, FILETIME systemTime = FILETIME(), DWORD pid = 0, const std::string& processName = "", const std::string& message = "");

	double time;
	FILETIME systemTime;
	DWORD pid;
	std::string processName;
	std::string message;
};

typedef std::vector<Line> Lines;

} // namespace debugviewpp 
} // namespace fusion
