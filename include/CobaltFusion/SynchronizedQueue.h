// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace fusion {

template <typename T>
class SynchronizedQueue
{
public:
    explicit SynchronizedQueue(size_t maxSize = 0) :
        m_maxSize(maxSize)
    {
    }

    bool Empty() const
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        return Empty(lock);
    }

    bool Full() const
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        return Full(lock);
    }

    size_t Size() const
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        return m_q.size();
    }

    size_t MaxSize() const
    {
        return m_maxSize;
    }

    void WaitForNotFull() const
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_cond.wait(lock, [&]() { return !Full(lock); });
    }

    template <typename Clock, typename Duration>
    bool WaitForNotFull(const std::chrono::time_point<Clock, Duration>& time) const
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        return m_cond.wait_until(lock, time, [&]() { return !Full(lock); });
    }

    void WaitForNotEmpty() const
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_cond.wait(lock, [&]() { return !Empty(lock); });
    }

    template <typename Clock, typename Duration>
    bool WaitForNotEmpty(const std::chrono::time_point<Clock, Duration>& time) const
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        return m_cond.wait_until(lock, time, [&]() { return !Empty(lock); });
    }

    void Push(T t)
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_cond.wait(lock, [&]() { return !Full(lock); });
        m_q.push(std::move(t));
        lock.unlock();
        m_cond.notify_one();
    }

    T Pop()
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_cond.wait(lock, [&]() { return !Empty(lock); });
        T t(m_q.front());
        m_q.pop();
        lock.unlock();
        m_cond.notify_one();
        return t;
    }

private:
    bool Empty(std::unique_lock<std::mutex>&) const
    {
        return m_q.empty();
    }

    bool Full(std::unique_lock<std::mutex>&) const
    {
        return m_maxSize > 0 && m_q.size() == m_maxSize;
    }

    size_t m_maxSize;
    mutable std::mutex m_mtx;
    mutable std::condition_variable m_cond;
    std::queue<T> m_q;
};

} // namespace fusion
