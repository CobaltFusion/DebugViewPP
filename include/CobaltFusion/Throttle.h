// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <chrono>
#include <functional>
#include "boost/optional.hpp"
#include "CobaltFusion/ExecutorClient.h"

namespace fusion {

class Throttle
{
public:
	using Clock = std::chrono::steady_clock;
	using Duration = Clock::duration;

	Throttle(IExecutor& executor, int callsPerSecond);
	void Call(std::function<void()> fn);
	void PendingCall(std::function<void()> fn);

private:
	Clock::duration m_delta;
	Clock::time_point m_lastCallTimePoint;
	Clock::time_point m_lastScheduledCallTimePoint;
	bool m_callPending;
	IExecutor& m_executor;
};

} // namespace fusion
