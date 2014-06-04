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

void LineBuffer::Add(double time, FILETIME systemTime, HANDLE handle, const char* message, LogSource* logsource)
{
	Write(time);
	Write(systemTime);
	Write(handle);
	WriteStringZ(message);
	Write(logsource);
}

InputLines LineBuffer::GetLines()
{
	InputLines lines;
	while (!Empty())
	{
		auto time = Read<double>();
		FILETIME systemTime = Read<FILETIME>();
		HANDLE processHandle = Read<HANDLE>();
		auto message = ReadStringZ();
		auto logsource = Read<LogSource*>();
		lines.push_back(InputLine(time, systemTime, processHandle, LineType::Normal, message, logsource));
	}
	NotifyWriter();
	return lines;
}

} // namespace debugviewpp
} // namespace fusion
