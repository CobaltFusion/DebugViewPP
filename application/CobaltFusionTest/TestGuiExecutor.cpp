// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TEST_NO_GUI_INIT
#include <boost/test/unit_test_gui.hpp>
#include "CobaltFusion/GuiExecutor.h"

using namespace std::chrono_literals;

namespace fusion {

BOOST_AUTO_TEST_SUITE(TestExecutor)

void TestGuiExecutorImpl(GuiExecutor& exec)
{
    using namespace std::chrono_literals;

    BOOST_CHECK_EQUAL(exec.Call([]() { return 1 + 1; }), 2);
    auto f = exec.CallAsync([]() { return 2 + 2; });
    BOOST_CHECK_EQUAL(f.get(), 4);

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

    int results[] = {0, 1, 2, 3, 4};
    BOOST_CHECK_EQUAL_COLLECTIONS(std::begin(vec), std::end(vec), std::begin(results), std::end(results));
}

BOOST_AUTO_TEST_CASE(TestGuiExecutor)
{
    using namespace std::chrono_literals;

    GuiExecutor exec;

    std::thread testThread([&exec]() { TestGuiExecutorImpl(exec); });

    bool elapsed = false;
    exec.CallAfter(1s, [&exec, &elapsed]() { elapsed = true; });

    GuiWaitFor([&exec, &elapsed]() { return elapsed; });

    testThread.join();
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
    BOOST_CHECK(task_mark == mark::immediate);
}

BOOST_AUTO_TEST_CASE(TestGuiExecutionOrder)
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
    BOOST_CHECK(task_mark == mark::immediate);
}

BOOST_AUTO_TEST_CASE(TestActiveExecutionOrder)
{
    ActiveExecutor executor;
    TestActiveExecutorOrderImpl(executor);
}
BOOST_AUTO_TEST_SUITE_END()

} // namespace fusion
