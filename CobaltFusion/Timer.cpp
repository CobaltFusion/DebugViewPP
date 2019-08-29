// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <stdexcept>
#include "CobaltFusion/Timer.h"

namespace fusion {

Timer::Timer() :
    m_timerUnit(0.0),
    m_init(false),
    m_offset(0)
{
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    if (li.QuadPart == 0)
    {
        throw std::runtime_error("QueryPerformanceCounter not supported!");
    }
    m_timerUnit = 1. / li.QuadPart;
}

void Timer::Reset()
{
    m_offset = 0;
    m_init = false;
}

double Timer::Get()
{
    auto ticks = GetTicks();
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_init)
    {
        m_offset = ticks;
        m_init = true;
    }
    return (ticks - m_offset) * m_timerUnit;
}

long long Timer::GetTicks()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

} // namespace fusion
