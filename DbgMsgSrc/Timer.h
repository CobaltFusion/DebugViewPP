#pragma once

#include <mutex>
#include <chrono>
#include <windows.h>

class Timer
{
public:
    Timer();
    std::chrono::steady_clock::time_point now() const;

    static double ToMs(std::chrono::steady_clock::duration duration)
    {
        return static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(duration).count()) / 1000.0;
    }

private:
    long long GetTicks() const;
    double m_timerUnit;
};
