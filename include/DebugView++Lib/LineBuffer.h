// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "CobaltFusion/CircularBuffer.h"
#include "Line.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer
{
public:
	virtual void Add(double time, FILETIME systemTime, HANDLE handle, const char* message) = 0;
	virtual InputLines GetLines() = 0;
};

class LineBuffer : public CircularBuffer, public ILineBuffer
{
public:
	explicit LineBuffer(size_t size);
	virtual ~LineBuffer() {}

	virtual void Add(double time, FILETIME systemTime, HANDLE handle, const char* message);
	virtual InputLines GetLines();
private:
	template <class T> T Read();
	template <class T> void Write(T value);
};

template <class T> 
T LineBuffer::Read()
{
	T value;
	auto p = (char*) &value;
	for (int i=0; i<sizeof(T); ++i)
		p[i] = CircularBuffer::Read();
	return value;
}

template <class T> 
void LineBuffer::Write(T value)
{
	auto p = (char*) &value;
	//std::cerr << "  store " << sizeof(T) << " bytes.\n";
	for (int i=0; i<sizeof(T); ++i)
	{
		if (Full())
			WaitForReader();
		CircularBuffer::Write(p[i]);
	}
}

} // namespace debugviewpp 
} // namespace fusion
