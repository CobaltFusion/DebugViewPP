// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <gtest/gtest.h>
#include <atomic>
#include <future>
#include "CobaltFusion/GuiExecutor.h"

using namespace std::chrono_literals;

namespace fusion {

void TestGuiExecutorImpl(GuiExecutor& exec)
{
    using namespace std::chrono_literals;

    EXPECT_EQ(exec.Call([]() { return 1 + 1; }), 2);
    auto f = exec.CallAsync([]() { return 2 + 2; });
    EXPECT_EQ(f.get(), 4);

    auto now = GuiExecutor::Clock::now();

    std::vector<int> vec;
    auto timer = exec.CallAt(now + 600ms, [&vec]() { vec.push_back(10); });
    exec.CallAt(now + 504ms, [&vec]() { vec.push_back(4); });
    exec.CallAt(now + 501ms, [&vec]() { vec.push_back(1); });
    exec.CallAt(now + 503ms, [&vec]() { vec.push_back(3); });
    exec.CallAt(now + 502ms, [&vec]() { vec.push_back(2); });
    exec.Call([&vec]() { vec.push_back(0); });

    // Cancel needs to excute within 600 ms. The 600ms needs to elapse within the 1 s sleep.
    // If not, the additional 10 will be appended to vec.
    // Longer delays help for test robustness but slows down testing.
    timer.Cancel();
    std::this_thread::sleep_for(1s);

    EXPECT_EQ(vec, (std::vector<int>{0, 1, 2, 3, 4}));
}

TEST(TestExecutor, TestGuiExecutor)
{
    using namespace std::chrono_literals;

    GuiExecutor exec;

    std::thread testThread([&exec]() { TestGuiExecutorImpl(exec); });

    bool elapsed = false;
    exec.CallAfter(1s, [&exec, &elapsed]() { elapsed = true; });

    GuiWaitFor([&exec, &elapsed]() { return elapsed; });

    testThread.join();
}

// Calls that schedule themselves again (several chains of follow-up calls, like the updates of the log view) must not keep the GUI thread from
// handling messages: DebugView++ froze for the duration of a flood of debug output because of that.
TEST(TestExecutor, TestGuiExecutorSelfRenewingCallsDoNotStarveTheMessageLoop)
{
    GuiExecutor exec;
    std::atomic<bool> stop{false};
    std::atomic<bool> finished{false};

    // 12 chains of 3 ms every 10 ms: there is always a call that is due
    std::function<void()> chain = [&] {
        std::this_thread::sleep_for(3ms);
        if (!stop)
        {
            exec.CallAfter(10ms, chain);
        }
    };
    for (int i = 0; i < 12; ++i)
    {
        exec.CallAfter(10ms, chain);
    }

    // a window of the GUI thread, the other thread sends it a message: that is only handled when the GUI thread gets back to its message loop
    HWND window = CreateWindowExW(0, L"STATIC", L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, nullptr);
    ASSERT_NE(window, nullptr);

    std::chrono::milliseconds worst = 0ms;
    std::thread testThread([&] {
        std::this_thread::sleep_for(200ms);
        for (int i = 0; i < 10; ++i)
        {
            const auto start = std::chrono::steady_clock::now();
            DWORD_PTR result = 0;
            if (SendMessageTimeoutW(window, WM_NULL, 0, 0, SMTO_NORMAL, 5000, &result) == 0)
            {
                worst = 5s; // starved, stop the chains so that the test can end
                break;
            }
            worst = std::max(worst, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start));
            std::this_thread::sleep_for(100ms);
        }
        stop = true;
        exec.CallAsync([&] { finished = true; });
    });

    GuiWaitFor([&] { return finished.load(); });
    testThread.join();
    DestroyWindow(window);

    EXPECT_LT(worst.count(), 500) << "the GUI thread did not handle a call for " << worst.count() << " ms";
}

void TestGuiExecutionOrderImpl(GuiExecutor& executor)
{
    enum class mark
    {
        unset,
        scheduled_call,
        immediate
    };

    mark task_mark = mark::unset;

    auto scheduled_call = executor.CallAfter(5min, [&] { task_mark = mark::scheduled_call; });
    executor.Call([&] { task_mark = mark::immediate; });
    scheduled_call.Cancel();
    EXPECT_EQ(task_mark, mark::immediate);
}

TEST(TestExecutor, TestGuiExecutionOrder)
{
    GuiExecutor executor;
    std::thread testThread([&executor]() { TestGuiExecutionOrderImpl(executor); });

    bool elapsed = false;
    executor.CallAfter(1s, [&executor, &elapsed]() { elapsed = true; });

    GuiWaitFor([&executor, &elapsed]() { return elapsed; });

    testThread.join();
}

void TestActiveExecutorOrderImpl(ActiveExecutor& executor)
{
    enum class mark
    {
        unset,
        scheduled_call,
        immediate
    };

    mark task_mark = mark::unset;

    auto scheduled_call = executor.CallAfter(5min, [&] { task_mark = mark::scheduled_call; });
    executor.Call([&] { task_mark = mark::immediate; });
    scheduled_call.Cancel();
    EXPECT_EQ(task_mark, mark::immediate);
}

TEST(TestExecutor, TestActiveExecutionOrder)
{
    ActiveExecutor executor;
    TestActiveExecutorOrderImpl(executor);
}

} // namespace fusion
