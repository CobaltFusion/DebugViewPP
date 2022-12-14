// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

// #include "stdafx.h"
#include "windows.h"

#pragma warning(disable : 4702 4389) //ignore signed/unsigned comparision and unreachable code in boost/test

// run as CobaltFusionTest.exe --log_level=test_suite

#define BOOST_TEST_MODULE CobaltFusionLib Unit Test
#include <boost/test/unit_test_gui.hpp>

#include <mutex>
#include <thread>
#include <chrono>
#include <random>
#include "CobaltFusion/CircularBuffer.h"
#include "CobaltFusion/Throttle.h"
#include "CobaltFusion/stringbuilder.h"
#include "CobaltFusion/tohex.h"

namespace fusion {

BOOST_AUTO_TEST_SUITE(ColbaltFusionLib)

class SynchronizedCircularBuffer
{
public:
    struct Timeout : std::runtime_error
    {
        Timeout() :
            std::runtime_error("SynchronizedCircularBuffer timeout")
        {
        }
    };

    explicit SynchronizedCircularBuffer(size_t capacity) :
        m_buffer(capacity)
    {
    }

    size_t Capacity() const
    {
        return m_buffer.Capacity();
    }

    bool Empty() const
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_buffer.Empty();
    }

    bool Full() const
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_buffer.Full();
    }

    size_t Available() const
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_buffer.Available();
    }

    size_t Size() const
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_buffer.Size();
    }

    char Read()
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_cond.wait(lock, [this] { return !m_buffer.Empty(); });
        char c = m_buffer.Read();
        lock.unlock();
        m_cond.notify_all();
        return c;
    }

    void Write(char c)
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_cond.wait(lock, [this] { return !m_buffer.Full(); });
        m_buffer.Write(c);
        lock.unlock();
        m_cond.notify_all();
    }

    void Clear()
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_buffer.Clear();
        lock.unlock();
        m_cond.notify_all();
    }

    void Swap(SynchronizedCircularBuffer& cb)
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        std::unique_lock<std::mutex> cbLock(cb.m_mtx);
        m_buffer.Swap(cb.m_buffer);
        cbLock.unlock();
        lock.unlock();
        cb.m_cond.notify_all();
        m_cond.notify_all();
    }

    void WriteStringZ(const std::string& s)
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_cond.wait(lock, [this, s] { return m_buffer.Available() > s.size(); });
        m_buffer.WriteStringZ(s.c_str());
        lock.unlock();
        m_cond.notify_all();
    }

    void WriteStringZ(const std::string& s, const std::chrono::system_clock::duration& timeout)
    {
        std::unique_lock<std::mutex> lock(m_mtx);
        if (!m_cond.wait_for(lock, timeout, [this, s] { return m_buffer.Available() > s.size(); }))
            throw Timeout();
        m_buffer.WriteStringZ(s.c_str());
        lock.unlock();
        m_cond.notify_all();
    }

    std::string ReadStringZ()
    {
        std::string s;
        while (auto c = Read())
            s += c;
        return s;
    }

private:
    mutable std::mutex m_mtx;
    mutable std::condition_variable m_cond;
    CircularBuffer m_buffer;
};

BOOST_AUTO_TEST_CASE(CircularBufferSize)
{
    CircularBuffer buffer(100);
    CircularBuffer buffer2(2 * 1024 * 1024);
    BOOST_CHECK_EQUAL(buffer2.Capacity(), 2 * 1024 * 1024);
}

BOOST_AUTO_TEST_CASE(CircularBufferInitialLevels)
{
    size_t testsize = 100;
    CircularBuffer buffer(testsize);
    BOOST_REQUIRE_EQUAL(testsize, buffer.Capacity());

    BOOST_CHECK(buffer.Empty());
    BOOST_CHECK(!buffer.Full());
    for (size_t i = 0; i < buffer.Capacity(); ++i)
        buffer.Write(1);

    BOOST_CHECK(!buffer.Empty());
    BOOST_CHECK(buffer.Full());

    for (size_t i = 0; i < buffer.Capacity(); ++i)
        buffer.Read();

    BOOST_CHECK(buffer.Empty());
    BOOST_CHECK(!buffer.Full());
}

BOOST_AUTO_TEST_CASE(CircularBufferCycle)
{
    size_t testsize = 100;
    CircularBuffer buffer(testsize);

    for (int j = 0; j < 1500; ++j)
    {
        BOOST_CHECK(buffer.Empty());
        for (int i = 0; i < 17; ++i)
            buffer.Write(1);

        BOOST_CHECK(!buffer.Empty());

        for (int i = 0; i < 17; ++i)
            buffer.Read();

        BOOST_CHECK(buffer.Empty());
    }
}

BOOST_AUTO_TEST_CASE(CircularBufferCycleStringZ)
{
    size_t testsize = 100;
    CircularBuffer buffer(testsize);

    for (int j = 0; j < 1000; ++j)
    {
        BOOST_CHECK(buffer.Empty());
        for (int i = 0; i < 17; ++i)
            buffer.WriteStringZ("test");

        BOOST_CHECK(!buffer.Empty());

        for (int i = 0; i < 17; ++i)
            BOOST_CHECK_EQUAL(buffer.ReadStringZ(), "test");

        BOOST_CHECK(buffer.Empty());
    }
}

BOOST_AUTO_TEST_CASE(CircularBufferCycleStringZPrime)
{
    size_t testsize = 200;
    CircularBuffer buffer(testsize);

    for (int j = 0; j < 500; ++j)
    {
        BOOST_CHECK(buffer.Empty());
        for (size_t i = 0; i < 17; ++i)
            buffer.WriteStringZ("test123");

        BOOST_CHECK(!buffer.Empty());

        for (int i = 0; i < 17; ++i)
            BOOST_CHECK_EQUAL(buffer.ReadStringZ(), "test123");

        BOOST_CHECK(buffer.Empty());
    }
}

BOOST_AUTO_TEST_CASE(CircularBufferBufferFullTimeout)
{
    size_t testsize = 80;
    SynchronizedCircularBuffer buffer(testsize);
    BOOST_CHECK(buffer.Empty());
    BOOST_CHECK(!buffer.Full());

    int iterations = 0;
    int writeIterations = 0;
    BOOST_CHECK_THROW(
        {
            for (int i = 0; i < 100; ++i)
            {
                ++iterations;
                buffer.WriteStringZ("test123", std::chrono::seconds(1));
                ++writeIterations;
                BOOST_CHECK(!buffer.Empty());
            }
        },
        std::exception);

    BOOST_TEST_MESSAGE("iterations: " << iterations);
    BOOST_CHECK(buffer.Full());

    int readInterations = 0;
    while (!buffer.Empty())
    {
        buffer.ReadStringZ();
        ++readInterations;
    }
    BOOST_CHECK(buffer.Empty());
    BOOST_CHECK(!buffer.Full());
    BOOST_CHECK_EQUAL(readInterations, writeIterations);
}

BOOST_AUTO_TEST_CASE(CircularBufferSwapping)
{
    size_t testsize = 30;
    CircularBuffer buffer(testsize);

    size_t testsize2 = 60;
    CircularBuffer buffer2(testsize2);

    buffer.WriteStringZ("test");
    buffer.WriteStringZ("test");
    buffer2.WriteStringZ("test");

    BOOST_REQUIRE_EQUAL(buffer.Size(), 10);
    BOOST_REQUIRE_EQUAL(buffer2.Size(), 5);

    buffer.Swap(buffer2);

    BOOST_REQUIRE_EQUAL(buffer.Size(), 5);
    BOOST_REQUIRE_EQUAL(buffer2.Size(), 10);
}

std::ostream& operator<<(std::ostream& os, const std::chrono::steady_clock::duration& p)
{
    using namespace std::chrono;
    return os << duration_cast<milliseconds>(p).count() << "ms";
}

// this test requires the 'Language for non-unicode programs' to be set to 'chinese-simplified'
// see Run "intl.cpl" -> Administrative -> 'Language for non-unicode programs'
//BOOST_AUTO_TEST_CASE(RoundTripUnicodeTest)
//{
//    auto t1 = ::setlocale(LC_ALL, "chinese-simplified");
//    BOOST_REQUIRE(t1 != nullptr);    // nulltr means 'could not set locale'
//
//    std::wstring chineseLanguage = L"\u4e2d\u6587"; // 中文";
//    std::string s = Str(chineseLanguage);
//    std::wstring w = WStr(s);
//    BOOST_REQUIRE(w == chineseLanguage);
//}

BOOST_AUTO_TEST_CASE(ThrottleTest)
{
    using namespace std::chrono_literals;
    using namespace std::chrono;
    const int testCPS = 20;

    ActiveExecutorClient exec;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> randomdelay(1, 90);
    auto start = ActiveExecutorClient::Clock::now();
    auto lastcallTime = start;
    auto lastexecutionTime = start;
    int incomingCounter = 0;
    int outgoingCounter = 0;

    auto fn = [&] {
        OutputDebugStringA("call...");
        lastexecutionTime = ActiveExecutorClient::Clock::now();
        outgoingCounter++;
    };

    Throttle throttledUpdateCounter(exec, testCPS, fn); // max calls per second

    for (int i = 0; i < 50; ++i)
    {
        for (int j = 0; j < 100000; ++j)
        {
            lastcallTime = ActiveExecutorClient::Clock::now();
            incomingCounter++;
            throttledUpdateCounter();
        }
        std::this_thread::sleep_for(1ms * randomdelay(gen));
    }

    // workaround to wait for any pending calls, since we dont have ExecutorClient::Flush()
    std::this_thread::sleep_for(200ms);

    auto testtime = ActiveExecutorClient::Clock::now() - start;
    auto testtestMs = duration_cast<milliseconds>(testtime).count();
    auto callsPerSecond = (1000 * outgoingCounter) / testtestMs;

    auto lastDelta = lastexecutionTime - lastcallTime;
    std::cout << "Incoming: " << incomingCounter << " calls, Outgoing: " << outgoingCounter << " calls, over " << testtime << " = " << callsPerSecond << "cps\n";
    std::cout << "Last execution was " << lastDelta << " after last call.\n";
    BOOST_CHECK_LT(callsPerSecond, testCPS);
    BOOST_CHECK_GT(lastDelta.count(), 0);
}


BOOST_AUTO_TEST_SUITE_END()

} // namespace fusion
