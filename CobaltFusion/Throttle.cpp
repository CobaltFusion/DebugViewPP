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
	m_delta(std::chrono::milliseconds(1000/callsPerSecond)),
	m_callPending(false)
{
}

void Throttle::Call(std::function<void()> fn)
{
	m_lastCallTimePoint = Clock::now();
	if (!m_callPending)
	{
		m_lastScheduledCallTimePoint = m_lastCallTimePoint;
		m_callPending = true;
		m_executor.CallAt(Clock::now() + m_delta, [this, fn] { PendingCall(fn); });
	}
}

void Throttle::PendingCall(std::function<void()> fn)
{
	fn();
	if (m_lastCallTimePoint > m_lastScheduledCallTimePoint)
	{
		m_lastScheduledCallTimePoint = Clock::now();
		m_executor.CallAt(Clock::now() + m_delta, [this, fn] { PendingCall(fn); });
	}
	else
	{
		m_callPending = false;
	}
}

} // namespace fusion
