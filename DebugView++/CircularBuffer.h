// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "DBWinBuffer.h"

namespace fusion {
namespace debugviewpp {

class CircularBuffer : boost::noncopyable
{
public:
	explicit CircularBuffer(size_t size);
	~CircularBuffer();

	void Add(double time, FILETIME systemTime, HANDLE handle, const char* message);
	Lines GetLines();
	
private:
	bool Empty();
	bool Full();
	void WaitForReader();

	template <class T> T Read();
	const char* ReadMessage();

	template <class T> void Write(T type);
	void WriteMessage(const char* message);

	char* m_pBegin;
	char* m_pEnd;
	const char* m_pRead;
	char* m_pWrite;

	boost::condition_variable m_triggerRead;
};

template <class T> 
T CircularBuffer::Read()
{
	auto pBuffer = (T *) m_pRead;
	m_pRead += sizeof(T);				// ! wrong, must wrap at the end of the buffer
	return *pBuffer;
}

template <class T> 
void CircularBuffer::Write(T value)
{
	auto pBuffer = (T *) m_pWrite;
	*pBuffer = value;					// ! wrong, must wrap at the end of the buffer
	m_pWrite += sizeof(T);
}

} // namespace debugviewpp 
} // namespace fusion
