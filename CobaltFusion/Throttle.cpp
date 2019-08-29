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

Throttle::Throttle(IExecutor& executor, int callsPerSecond, std::function<void()> fn) :
    m_delta(std::chrono::milliseconds(1000 / callsPerSecond)),
    m_callPending(false),
    m_fn(std::move(fn)),
    m_executor(executor)
{
}

void Throttle::operator()()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastCallTimePoint = Clock::now();
    if (!m_callPending)
    {
        m_lastScheduledCallTimePoint = m_lastCallTimePoint;
        m_callPending = true;
        m_executor.CallAt(Clock::now() + m_delta, [this] { PendingCall(); });
    }
}

void Throttle::PendingCall()
{
    m_fn();
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_lastCallTimePoint > m_lastScheduledCallTimePoint)
    {
        m_lastScheduledCallTimePoint = Clock::now();
        m_executor.CallAt(Clock::now() + m_delta, [this] { PendingCall(); });
    }
    else
    {
        m_callPending = false;
    }
}

} // namespace fusion
