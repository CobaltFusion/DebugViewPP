// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <chrono>
#include "DebugView++Lib/Throttle.h"

namespace fusion {
namespace debugviewpp {

using namespace std::chrono_literals;


Throttle::Throttle(IExecutor& executor, std::function<bool()> fn, int callsPerSecond) :
	m_executor(executor),
	m_func(fn),
	m_delta(std::chrono::milliseconds(1000/callsPerSecond))
{
}

void Throttle::operator()()
{
	if (m_scheduledCall)
	{
		if (Clock::now() > m_lastScheduleCallTimePoint)
		{
			m_scheduledCall.get().Cancel();
			m_executor.CallAsync([&] { m_func(); });
		}
	}
	else
	{
		m_lastScheduleCallTimePoint = Clock::now() + m_delta;
		m_scheduledCall = m_executor.CallAt(m_lastScheduleCallTimePoint, [&] { m_func(); });
	}
}

} // namespace debugviewpp 
} // namespace fusion
