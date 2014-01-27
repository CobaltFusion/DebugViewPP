// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "stdafx.h"
#include "LineBuffer.h"
#include "dbgstream.h"

namespace fusion {
namespace debugviewpp {

LineBuffer::LineBuffer(size_t size) : CircularBuffer(size)
{
}

bool LineBuffer::Full() const
{
	const long maxMessageSize = sizeof(double) + sizeof(FILETIME) + sizeof(HANDLE) + sizeof(DbWinBuffer) + 1;
	return (maxMessageSize > GetFree());
}

void LineBuffer::Add(double time, FILETIME systemTime, HANDLE handle, const char* message)
{
	if (Full())
		WaitForReader();
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
		//Handle handle;
		auto time = Read<double>();
		auto filetime = Read<FILETIME>();
		auto processHandle = Read<HANDLE>();
		auto message = ReadStringZ();
		DWORD pid = 0;			
		std::string processName = "process";
		lines.push_back(Line(time, filetime, pid, processName, message));
		printf("got line\n");
		printStats();
	}
	m_triggerRead.notify_all();
	return Lines();
}

} // namespace debugviewpp
} // namespace fusion
