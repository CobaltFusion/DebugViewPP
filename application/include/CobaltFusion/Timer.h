// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <mutex>
#include <Windows.h>

namespace fusion {

long long GetTicks();

class Timer
{
public:
    Timer();

    void Reset();
    double Get();
    double GetTimeSince(long long);

private:
    double m_timerUnit;
    bool m_init;
    long long m_offset;
    std::mutex m_mutex;
};

} // namespace fusion
