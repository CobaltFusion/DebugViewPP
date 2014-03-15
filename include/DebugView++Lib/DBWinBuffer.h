// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <vector>

namespace fusion {
namespace debugviewpp {

struct DbWinBuffer
{
	DWORD processId;
	// Total size must be 4KB (processID + data)
	char data[4096 - sizeof(DWORD)];
};

static_assert(sizeof(DbWinBuffer) == 4096, "DBWIN_BUFFER size must be 4096");

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

bool IsWindowsVistaOrGreater();
bool IsDBWinViewerActive();
bool HasGlobalDBWinReaderRights();

} // namespace debugviewpp 
} // namespace fusion
