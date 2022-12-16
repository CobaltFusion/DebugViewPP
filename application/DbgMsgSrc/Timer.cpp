#include "Timer.h"

#include <stdexcept>
#include <chrono>

#include "windows.h"

#define MICROSECONDS (1e6 / li.QuadPart)

Timer::Timer() :
    m_timerUnit(0.0)
{
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    if (li.QuadPart == 0)
        throw std::runtime_error("QueryPerformanceCounter not supported!");
    m_timerUnit = MICROSECONDS;
}

std::chrono::steady_clock::time_point Timer::now() const
{
    return std::chrono::steady_clock::time_point(std::chrono::microseconds(static_cast<long long>(GetTicks() * m_timerUnit)));
}

long long Timer::GetTicks() const
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}
