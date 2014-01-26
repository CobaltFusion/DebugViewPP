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
#include "dbgstream.h"

namespace fusion {
namespace debugviewpp {

// see http://en.wikipedia.org/wiki/Circular_buffer 
// the 'Always Keep One Slot Open' strategy is used to distigush between empty and full conditions

// +---+---+---+---+---+---+---+---+ 
// | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |    Empty Buffer
// +---+----+--+---+---+---+---+---+ 
//               R
//               W

// +---+---+---+---+---+---+---+---+ 
// | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |    1 Bytes in buffer
// +---+----+--+---+---+---+---+---+ 
//               R   W 
//

// +---+---+---+---+---+---+---+---+ 
// | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |    2 Bytes in buffer
// +---+----+--+---+---+---+---+---+ 
//               R       W 
//

// +---+---+---+---+---+---+---+---+ 
// | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |    2 Bytes in buffer
// +---+----+--+---+---+---+---+---+ 
//       W                       R
//

// +---+---+---+---+---+---+---+---+ 
// | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |    Full buffer (1 byte used as 'open slot')
// +---+---+---+---+---+---+---+---+ 
//           W   R

// +---+---+---+---+---+---+---+---+ 
// | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |    Full buffer (1 byte used as 'open slot')
// +---+---+---+---+---+---+---+---+ 
//   R                           W 


CircularBuffer::CircularBuffer(size_t size) :
	m_size(GetPowerOfTwo(size)),
	m_buffer(new char[m_size]),
	m_pBegin(m_buffer.get()),
	m_pEnd(m_pBegin + m_size),
	m_readOffset(0),
	m_writeOffset(0)
{
}

int CircularBuffer::GetPowerOfTwo(int v)
{
	v--;	// decrement by one, so if the input is a power of two, we return the input value.
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}
	
bool CircularBuffer::Empty() const
{
	return m_readOffset == m_writeOffset;
}

size_t CircularBuffer::GetFree() const
{
	return (m_writeOffset < m_readOffset)
                ? (m_readOffset - m_writeOffset) - 1
                : (m_size - m_writeOffset) + m_readOffset - 1;
}

size_t CircularBuffer::GetCount() const
{
	return (m_writeOffset >= m_readOffset)
                ? m_writeOffset - m_readOffset
                : (m_size - m_readOffset) + m_writeOffset;
}

bool CircularBuffer::Full() const
{
	const long maxMessageSize = sizeof(double) + sizeof(FILETIME) + sizeof(HANDLE) + sizeof(DbWinBuffer) + 1;
	return (maxMessageSize > GetFree());
	//return PtrAdd(m_writeOffset, 1) == m_readOffset; // actually full
}

std::string CircularBuffer::ReadMessage()
{
	auto pBuffer = m_buffer.get() + m_readOffset;
	std::string message(pBuffer);
	m_readOffset = PtrAdd(m_readOffset, message.length()+1);
	return message;
}

void CircularBuffer::WriteMessage(const char* message)
{
	for (size_t i=0; i < strlen(message); ++i)
		Write(message[i]);
	Write(char(0));
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

void CircularBuffer::printStats()
{
	cdbg << "Full: " << (Full() ? "yes" : "no") << "\n";
	cdbg << "Empty: " << (Empty() ? "yes" : "no") << "\n";
	cdbg << "Count: " << GetCount() << "\n";

	printf("size: %d\t", m_size);
	printf("Full: %s\t",  (Full() ? "yes" : "no"));
	printf("Empty: %s\t",  (Empty() ? "yes" : "no"));
	printf("Count: %d\t",  GetCount());
	printf("m_readOffset: %d\t", m_readOffset);
	printf("m_writeOffset: %d\n", m_writeOffset);
}

void CircularBuffer::WaitForReader()
{
    auto predicate = [this] { return !Full(); };

	while (Full())
	{
		boost::mutex waitingLock;
		boost::unique_lock<boost::mutex> lock(waitingLock);
		bool result = m_triggerRead.timed_wait(lock, boost::posix_time::seconds(1), predicate);
		if (!result)
		{
			throw std::exception("timeout");	// only so I can test without multiple threads
		}
	}
}

Lines CircularBuffer::GetLines()
{
	Lines lines;
	while (!Empty())
	{
		//Handle handle;
		auto time = Read<double>();
		auto filetime = Read<FILETIME>();
		auto processHandle = Read<HANDLE>();
		auto message = ReadMessage();
		DWORD pid = 0;			
		std::string processName = "process";
		lines.push_back(Line(time, filetime, pid, processName, message));
		printf("got line\n");
		printStats();
	}
	m_triggerRead.notify_all();
	return Lines();
}

CircularBuffer::~CircularBuffer()
{
}

} // namespace debugviewpp
} // namespace fusion
