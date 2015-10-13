// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include <cmath>
#include <boost/date_time/local_time/local_time.hpp> 
#pragma warning(push, 3)
#include <boost/thread.hpp>
#pragma warning(pop)
#include "Win32/Win32Lib.h"

namespace fusion {

template <typename T, size_t N>
size_t array_sizeX(T (&)[N])
{
	return N;
}

std::wstring LoadString(int id);

std::wstring GetExceptionMessage();

class Timer
{
public:
	Timer();
	Timer(LONGLONG quadPart);

	void Reset();
	double Get(long long ticks);
	double Get();

private:
	long long GetTicks() const;

	double m_timerUnit;
	bool m_init;
	long long m_offset;
	boost::mutex m_mutex;
};

template <typename T>
T floor_to(double value)
{
	return static_cast<T>(std::floor(value));
}

FILETIME MakeFileTime(uint64_t t);

} // namespace fusion
