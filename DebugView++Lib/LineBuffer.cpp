// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "stdafx.h"
#include "DebugView++Lib/LineBuffer.h"

namespace fusion {
namespace debugviewpp {

LineBuffer::LineBuffer(size_t size) : CircularBuffer(size)
{
}

void LineBuffer::Add(double time, FILETIME systemTime, HANDLE handle, const char* message)
{
	Write(time);
	Write(systemTime);
	Write(handle);
	WriteStringZ(message);
}

Lines LineBuffer::GetLines()
{
	Lines lines;
	while (!Empty())
	{
		auto _time = Read<double>();
		FILETIME _systemTime = Read<FILETIME>();
		HANDLE _processHandle = Read<HANDLE>();
		auto _message = ReadStringZ();
		DWORD pid = GetProcessId(_processHandle);		
		std::string processName = "process";
		lines.push_back(Line(_time, _systemTime, pid, processName, _message));
	}
	NotifyWriter();
	return lines;
}

} // namespace debugviewpp
} // namespace fusion
