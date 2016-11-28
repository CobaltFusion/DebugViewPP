// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <mutex>
#include <Windows.h>

namespace fusion {

class Timer
{
public:
	Timer();

	void Reset();
	double Get();

private:
	long long GetTicks() const;

	double m_timerUnit;
	bool m_init;
	long long m_offset;
	std::mutex m_mutex;
};

} // namespace fusion
