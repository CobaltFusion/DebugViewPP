// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <chrono>
#include <iostream>
#include "CobaltFusion/Throttle.h"
#include "CobaltFusion/stringbuilder.h"

namespace fusion {

using namespace std::chrono_literals;

Throttle::Throttle(IExecutor& executor, int callsPerSecond) :
	m_executor(executor),
	m_delta(std::chrono::milliseconds(1000/callsPerSecond))
{
}

void Throttle::Call(std::function<void()> fn)
{
	if (m_scheduledCall.is_initialized())
	{
		if ((Clock::now() - m_lastScheduleCallTimePoint) > m_delta /2 )
		{
			m_scheduledCall.get().Cancel();
			m_scheduledCall.reset();
			m_executor.CallAsync([=] { fn(); });
		}
	}
	else
	{
		m_lastScheduleCallTimePoint = Clock::now() + m_delta;
		m_scheduledCall = m_executor.CallAt(m_lastScheduleCallTimePoint, [=] { fn(); });
	}
}

} // namespace fusion
