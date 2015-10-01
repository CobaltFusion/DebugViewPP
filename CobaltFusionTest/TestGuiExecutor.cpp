// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#define BOOST_TEST_NO_GUI_INIT
#include <boost/test/unit_test_gui.hpp>
#include "CobaltFusion/GuiExecutor.h"

namespace fusion {

BOOST_AUTO_TEST_SUITE(TestExecutor)

void TestGuiExecutorImpl(GuiExecutor& exec)
{
	BOOST_CHECK_EQUAL(exec.Call([]() { return 1 + 1; }), 2);
	auto f = exec.CallAsync([]() { return 2 + 2; });
	BOOST_CHECK_EQUAL(f.get(), 4);

	auto now = boost::chrono::steady_clock::now();

	std::vector<int> vec;
	auto timer = exec.CallAt(now + boost::chrono::milliseconds(600), [&vec]() { vec.push_back(10); });
	exec.CallAt(now + boost::chrono::milliseconds(504), [&vec]() { vec.push_back(4); });
	exec.CallAt(now + boost::chrono::milliseconds(501), [&vec]() { vec.push_back(1); });
	exec.CallAt(now + boost::chrono::milliseconds(503), [&vec]() { vec.push_back(3); });
	exec.CallAt(now + boost::chrono::milliseconds(502), [&vec]() { vec.push_back(2); });
	exec.Call([&vec]() { vec.push_back(0); });

	// Cancel needs to excute within 600 ms. The 600ms needs to elapse within the 1 s sleep.
	// If not, the additional 10 will be appended to vec.
	// Longer delays help for test robustness but slows down testing.
	timer.Cancel();
	boost::this_thread::sleep_for(boost::chrono::seconds(1));

	int results[] = { 0, 1, 2, 3, 4 };
	BOOST_CHECK_EQUAL_COLLECTIONS(std::begin(vec), std::end(vec), std::begin(results), std::end(results));
}

BOOST_AUTO_TEST_CASE(TestGuiExecutor)
{
	GuiExecutor exec;

	boost::thread testThread([&exec]() { TestGuiExecutorImpl(exec); });

	bool elapsed = false;
	exec.CallAfter(boost::chrono::seconds(1), [&exec, &elapsed]() { elapsed = true; });
	
	GuiWaitFor([&exec, &elapsed]() { return elapsed; });

	testThread.join();
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace fusion
