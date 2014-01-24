// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#define _SCL_SECURE_NO_WARNINGS

#include "stdafx.h"
#include "CircularBuffer.h"
#include "DBWinBuffer.h"
#include "Win32Lib.h"

namespace fusion {
namespace debugviewpp {

CircularBuffer::CircularBuffer(size_t size) :
	m_pBegin(new char[size]),
	m_pEnd(m_pBegin + size),
	m_pRead(m_pBegin),
	m_pWrite(m_pBegin)
{
}
	
bool CircularBuffer::Empty()
{
	return false;
}

bool CircularBuffer::Full()
{
	const long maxMessageSize = sizeof(double) + sizeof(FILETIME) + sizeof(HANDLE) + sizeof(DbWinBuffer);

	//	return (m_pWrite >= (m_pEnd-maxMessageSize));	// ! wrong, must wrap at the end of the buffer

	// todo: create either operators or methods to do CircularBuffer arithmetic
	return false;
}

const char* CircularBuffer::ReadMessage()
{
	return 0;
}

void CircularBuffer::WriteMessage(const char* message)
{
	size_t len = strlen(message);
	std::copy(message, message+len, stdext::checked_array_iterator<char*>(m_pWrite, m_pEnd-m_pWrite));
	m_pWrite += len;
}

void CircularBuffer::Add(double time, FILETIME systemTime, HANDLE handle, const char* message)
{
	if (Full())
		WaitForReader();
	Write(time);
	Write(systemTime);
	Write(handle);
	WriteMessage(message);
}

void CircularBuffer::WaitForReader()
{
    auto predicate = [this] { return !Full(); };

	while (Full())
	{
		boost::mutex waitingLock;
		boost::unique_lock<boost::mutex> lock(waitingLock);
		m_triggerRead.timed_wait(lock, boost::posix_time::seconds(1), predicate);
	}
}

Lines CircularBuffer::GetLines()
{
	Lines lines;
	while (!Empty())
	{
		Handle handle(Read<HANDLE>());
		DWORD pid = 0;			
		std::string processName;
		lines.push_back(Line(Read<double>(), Read<FILETIME>(), pid, processName, ReadMessage()));
	}
	m_triggerRead.notify_all();
	return Lines();
}

CircularBuffer::~CircularBuffer()
{
	delete[] m_pBegin;
}

} // namespace debugviewpp
} // namespace fusion
