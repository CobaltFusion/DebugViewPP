// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <chrono>
#include "CobaltFusion/Throttle.h"
#include "CobaltFusion/stringbuilder.h"

namespace fusion {

using namespace std::chrono_literals;

Throttle::Throttle(IExecutor& executor, int callsPerSecond, std::function<void()> fn) :
    m_delta(1000ms / callsPerSecond),
    m_callPending(false),
    m_fn(std::move(fn)),
    m_executor(executor)
{
}

void Throttle::operator()()
{
    auto now = Clock::now();
    std::unique_lock<std::mutex> lock(m_mutex);
    m_lastCallTime = now;
    if (!m_callPending)
    {
        m_lastSchedulingTime = now;
        m_callPending = true;
        lock.unlock();
        m_executor.CallAt(now + m_delta, [this] { PendingCall(); });
    }
}

void Throttle::PendingCall()
{
    m_fn();
    auto now = Clock::now();
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_lastCallTime > m_lastSchedulingTime)
    {
        m_lastSchedulingTime = now;
        lock.unlock();
        m_executor.CallAt(now + m_delta, [this] { PendingCall(); });
    }
    else
    {
        m_callPending = false;
    }
}

} // namespace fusion
