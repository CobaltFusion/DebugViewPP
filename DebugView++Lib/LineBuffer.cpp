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
		Loopback,			// time, system, pid and processname are available
	};
};

LineBuffer::LineBuffer(size_t size) : CircularBuffer(size)
{
}

void LineBuffer::Add(double time, FILETIME systemTime, HANDLE handle, const char* message, LogSource* logsource)
{
	Write<unsigned char>(LineType::DBWinMessage);
	Write(time);
	Write(systemTime);
	Write(handle);
	WriteStringZ(message);
	Write(logsource);
}

void LineBuffer::Add(double time, FILETIME systemTime, DWORD pid, const char* processName, const char* message, LogSource* logsource)
{
	Write<unsigned char>(LineType::Loopback);
	Write(time);
	Write(systemTime);
	Write(pid);
	WriteStringZ(processName);
	WriteStringZ(message);
	Write(logsource);
}

Lines LineBuffer::GetLines()
{
	//DumpStats();
	Lines lines;
	while (!Empty())
	{
		auto type = (LineType::type) Read<unsigned char>();
		if (type == LineType::DBWinMessage)
		{
			auto time = Read<double>();
			FILETIME systemTime = Read<FILETIME>();
			HANDLE processHandle = Read<HANDLE>();
			auto message = ReadStringZ();
			auto logsource = Read<LogSource*>();
			lines.push_back(Line(time, systemTime, processHandle, message, logsource));
		}
		else
		{
			auto time = Read<double>();
			FILETIME systemTime = Read<FILETIME>();
			DWORD processId = Read<DWORD>();
			auto processName = ReadStringZ();
			auto message = ReadStringZ();
			auto logsource = Read<LogSource*>();
			lines.push_back(Line(time, systemTime, processId, processName, message, logsource));
		}
		//DumpStats();
	}
	NotifyWriter();
	return lines;
}

} // namespace debugviewpp
} // namespace fusion
