// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "CobaltFusion/CircularBuffer.h"
#include "DBWinBuffer.h"

namespace fusion {
namespace debugviewpp {

class LineBuffer : public CircularBuffer
{
public:
	explicit LineBuffer(size_t size);

	void Add(double time, FILETIME systemTime, HANDLE handle, const char* message);
	Lines GetLines();
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
		*p = CircularBuffer::Read();
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
