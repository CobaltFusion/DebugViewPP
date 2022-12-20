// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <chrono>
#include <functional>
#include <optional>
#include "CobaltFusion/ExecutorClient.h"

namespace fusion {

class Throttle
{
public:
    using Clock = std::chrono::steady_clock;
    using Duration = Clock::duration;

    Throttle(IExecutor& executor, int callsPerSecond, std::function<void()> fn);

    void operator()();
    void PendingCall();

private:
    Clock::duration m_delta;
    Clock::time_point m_lastCallTime;
    Clock::time_point m_lastSchedulingTime;
    bool m_callPending;
    std::function<void()> m_fn;
    std::mutex m_mutex;
    IExecutor& m_executor;
};

} // namespace fusion
