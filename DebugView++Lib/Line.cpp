// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Win32/Win32Lib.h"
#include "DebugView++Lib/Line.h"

namespace fusion {
namespace debugviewpp {

Line::Line(double time, FILETIME systemTime, HANDLE handle, const std::string& message, const LogSource* pLogSource) :
    time(time),
    systemTime(systemTime),
    handle(handle),
    pid(0),
    message(message),
    pLogSource(pLogSource)
{
}

Line::Line(double time, FILETIME systemTime, DWORD pid, const std::string& processName, const std::string& message, const LogSource* pLogSource) :
    time(time),
    systemTime(systemTime),
    handle(nullptr),
    pid(pid),
    processName(processName),
    message(message),
    pLogSource(pLogSource)
{
}

} // namespace debugviewpp
} // namespace fusion
