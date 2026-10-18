// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "windows.h"

// run as CobaltFusionTest.exe, or with --gtest_filter=Suite.Test and --gtest_list_tests

#include <gtest/gtest.h>

#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <random>
#include "CobaltFusion/CircularBuffer.h"
#include "CobaltFusion/Signal.h"
#include "CobaltFusion/Throttle.h"
#include "CobaltFusion/Timer.h"
#include "CobaltFusion/stringbuilder.h"
#include "CobaltFusion/tohex.h"

namespace fusion {

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

TEST(ColbaltFusionLib, CircularBufferSize)
{
    CircularBuffer buffer(100);
    CircularBuffer buffer2(2 * 1024 * 1024);
    EXPECT_EQ(buffer2.Capacity(), 2 * 1024 * 1024);
}

TEST(ColbaltFusionLib, CircularBufferInitialLevels)
{
    size_t testsize = 100;
    CircularBuffer buffer(testsize);
    ASSERT_EQ(testsize, buffer.Capacity());

    EXPECT_TRUE(buffer.Empty());
    EXPECT_FALSE(buffer.Full());
    for (size_t i = 0; i < buffer.Capacity(); ++i)
        buffer.Write(1);

    EXPECT_FALSE(buffer.Empty());
    EXPECT_TRUE(buffer.Full());

    for (size_t i = 0; i < buffer.Capacity(); ++i)
        buffer.Read();

    EXPECT_TRUE(buffer.Empty());
    EXPECT_FALSE(buffer.Full());
}

TEST(ColbaltFusionLib, CircularBufferCycle)
{
    size_t testsize = 100;
    CircularBuffer buffer(testsize);

    for (int j = 0; j < 1500; ++j)
    {
        EXPECT_TRUE(buffer.Empty());
        for (int i = 0; i < 17; ++i)
            buffer.Write(1);

        EXPECT_FALSE(buffer.Empty());

        for (int i = 0; i < 17; ++i)
            buffer.Read();

        EXPECT_TRUE(buffer.Empty());
    }
}

TEST(ColbaltFusionLib, CircularBufferCycleStringZ)
{
    size_t testsize = 100;
    CircularBuffer buffer(testsize);

    for (int j = 0; j < 1000; ++j)
    {
        EXPECT_TRUE(buffer.Empty());
        for (int i = 0; i < 17; ++i)
            buffer.WriteStringZ("test");

        EXPECT_FALSE(buffer.Empty());

        for (int i = 0; i < 17; ++i)
            EXPECT_EQ(buffer.ReadStringZ(), "test");

        EXPECT_TRUE(buffer.Empty());
    }
}

TEST(ColbaltFusionLib, CircularBufferCycleStringZPrime)
{
    size_t testsize = 200;
    CircularBuffer buffer(testsize);

    for (int j = 0; j < 500; ++j)
    {
        EXPECT_TRUE(buffer.Empty());
        for (size_t i = 0; i < 17; ++i)
            buffer.WriteStringZ("test123");

        EXPECT_FALSE(buffer.Empty());

        for (int i = 0; i < 17; ++i)
            EXPECT_EQ(buffer.ReadStringZ(), "test123");

        EXPECT_TRUE(buffer.Empty());
    }
}

TEST(ColbaltFusionLib, CircularBufferBufferFullTimeout)
{
    size_t testsize = 80;
    SynchronizedCircularBuffer buffer(testsize);
    EXPECT_TRUE(buffer.Empty());
    EXPECT_FALSE(buffer.Full());

    int iterations = 0;
    int writeIterations = 0;
    EXPECT_THROW({
            for (int i = 0; i < 100; ++i)
            {
                ++iterations;
                buffer.WriteStringZ("test123", std::chrono::seconds(1));
                ++writeIterations;
                EXPECT_FALSE(buffer.Empty());
            }
        }, std::exception);

    SCOPED_TRACE(::testing::Message() << "iterations: " << iterations);
    EXPECT_TRUE(buffer.Full());

    int readInterations = 0;
    while (!buffer.Empty())
    {
        buffer.ReadStringZ();
        ++readInterations;
    }
    EXPECT_TRUE(buffer.Empty());
    EXPECT_FALSE(buffer.Full());
    EXPECT_EQ(readInterations, writeIterations);
}

TEST(ColbaltFusionLib, CircularBufferSwapping)
{
    size_t testsize = 30;
    CircularBuffer buffer(testsize);

    size_t testsize2 = 60;
    CircularBuffer buffer2(testsize2);

    buffer.WriteStringZ("test");
    buffer.WriteStringZ("test");
    buffer2.WriteStringZ("test");

    ASSERT_EQ(buffer.Size(), 10);
    ASSERT_EQ(buffer2.Size(), 5);

    buffer.Swap(buffer2);

    ASSERT_EQ(buffer.Size(), 5);
    ASSERT_EQ(buffer2.Size(), 10);
}

std::ostream& operator<<(std::ostream& os, const std::chrono::steady_clock::duration& p)
{
    using namespace std::chrono;
    return os << duration_cast<milliseconds>(p).count() << "ms";
}

TEST(ColbaltFusionLib, RoundTripUnicodeTest)
{
    std::wstring unicodeSample = L"Hello, 世界! é/Ω/€ 🚀";
    std::string s = Str(unicodeSample);
    std::wstring w = WStr(s);
    ASSERT_EQ(w, unicodeSample);

    std::string sWithBom = "\xEF\xBB\xBF" + s;
    std::wstring wStripped = WStr(sWithBom);
    ASSERT_EQ(wStripped, unicodeSample);
}

TEST(ColbaltFusionLib, ThrottleTest)
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
    EXPECT_LT(callsPerSecond, testCPS);
    EXPECT_GT(lastDelta.count(), 0);
}

TEST(ColbaltFusionLib, TimerTest)
{
    Timer t;
}

TEST(ColbaltFusionLib, SignalCallsSlotsInConnectionOrder)
{
    Signal<void(int)> signal;
    std::vector<int> calls;
    signal.connect([&](int v) { calls.push_back(v * 10 + 1); });
    signal.connect([&](int v) { calls.push_back(v * 10 + 2); });
    signal.connect([&](int v) { calls.push_back(v * 10 + 3); });
    signal(4);
    EXPECT_EQ(calls, (std::vector<int>{41, 42, 43}));
}

TEST(ColbaltFusionLib, SignalWithoutSlotsIsHarmless)
{
    Signal<void()> signal;
    EXPECT_TRUE(signal.empty());
    signal();

    Signal<int()> withResult;
    EXPECT_EQ(withResult(), 0);
}

TEST(ColbaltFusionLib, SignalReturnsTheResultOfTheLastSlot)
{
    Signal<bool()> signal;
    signal.connect([] { return true; });
    signal.connect([] { return false; });
    EXPECT_FALSE(signal());
    signal.connect([] { return true; });
    EXPECT_TRUE(signal());
}

TEST(ColbaltFusionLib, SignalDisconnect)
{
    Signal<void()> signal;
    int a = 0;
    int b = 0;
    auto ca = signal.connect([&] { ++a; });
    signal.connect([&] { ++b; });
    EXPECT_TRUE(ca.connected());

    signal();
    ca.disconnect();
    EXPECT_FALSE(ca.connected());
    ca.disconnect(); // twice is fine
    signal();

    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 2);

    Connection none;
    EXPECT_FALSE(none.connected());
    none.disconnect();
}

TEST(ColbaltFusionLib, SignalDisconnectAllSlots)
{
    Signal<void()> signal;
    int calls = 0;
    auto c = signal.connect([&] { ++calls; });
    signal.disconnect_all_slots();
    signal();
    EXPECT_EQ(calls, 0);
    EXPECT_FALSE(c.connected());
    EXPECT_TRUE(signal.empty());
}

TEST(ColbaltFusionLib, SignalConnectionCanOutliveTheSignal)
{
    Connection c;
    {
        Signal<void()> signal;
        c = signal.connect([] {});
        EXPECT_TRUE(c.connected());
    }
    c.disconnect();
    EXPECT_FALSE(c.connected());
}

TEST(ColbaltFusionLib, SignalSlotCanDisconnectItselfAndConnectOthers)
{
    Signal<void()> signal;
    int first = 0;
    int added = 0;
    Connection self;
    self = signal.connect([&] {
        ++first;
        self.disconnect();
        signal.connect([&] { ++added; }); // takes effect from the next call
    });

    signal();
    EXPECT_EQ(first, 1);
    EXPECT_EQ(added, 0);
    signal();
    EXPECT_EQ(first, 1);
    EXPECT_EQ(added, 1);
}

TEST(ColbaltFusionLib, SignalConcurrentEmitAndConnect)
{
    Signal<void()> signal;
    std::atomic<int> calls{0};
    std::atomic<bool> stop{false};
    signal.connect([&] { ++calls; });

    std::thread emitter1([&] { while (!stop) { signal(); } });
    std::thread emitter2([&] { while (!stop) { signal(); } });
    std::thread connector([&] {
        for (int i = 0; i < 2000; ++i)
        {
            auto c = signal.connect([&] { ++calls; });
            c.disconnect();
        }
    });

    connector.join();
    stop = true;
    emitter1.join();
    emitter2.join();
    EXPECT_GT(calls, 0);
    EXPECT_FALSE(signal.empty());
}

} // namespace fusion
