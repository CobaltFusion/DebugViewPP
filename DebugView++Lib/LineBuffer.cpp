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

struct LineType
{
	enum type
	{
		DBWinMessage,		// process handle is available
		Logfile				// pid and processname are available
	};
};

LineBuffer::LineBuffer(size_t size) : CircularBuffer(size)
{
}

void LineBuffer::Add(double time, FILETIME systemTime, HANDLE handle, const char* message, LogSource* logsource)
{
	Write(LineType::DBWinMessage);
	Write(time);
	Write(systemTime);
	Write(handle);
	WriteStringZ(message);
	Write(logsource);
}

void LineBuffer::Add(double time, FILETIME systemTime, DWORD pid, const char* processName, const char* message, LogSource* logsource)
{
	Write<unsigned char>(LineType::Logfile);
	Write(time);
	Write(systemTime);
	Write(pid);
	WriteStringZ(processName);
	WriteStringZ(message);
	Write(logsource);
}

Lines LineBuffer::GetLines()
{
	Lines lines;
	while (!Empty())
	{
		auto type = (LineType::type) Read<unsigned char>();
		auto time = Read<double>();
		FILETIME systemTime = Read<FILETIME>();
		if (type == LineType::DBWinMessage)
		{
			HANDLE processHandle = Read<HANDLE>();
			auto message = ReadStringZ();
			auto logsource = Read<LogSource*>();
			lines.push_back(Line(time, systemTime, processHandle, message, logsource));
		}
		else
		{
			DWORD processId = Read<DWORD>();
			auto processName = ReadStringZ();
			auto message = ReadStringZ();
			auto logsource = Read<LogSource*>();
			lines.push_back(Line(time, systemTime, processId, processName, message, logsource));
		}
	}
	NotifyWriter();
	return lines;
}

} // namespace debugviewpp
} // namespace fusion
