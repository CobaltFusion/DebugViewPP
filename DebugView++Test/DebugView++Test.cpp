// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"

#define BOOST_TEST_MODULE DebugView++Lib Unit Test
#include <boost/test/unit_test_gui.hpp>
#include <random>

#include "Win32/Utilities.h"
#include "Win32/Win32Lib.h"
#include "CobaltFusion/stringbuilder.h"
#include "IndexedStorageLib/IndexedStorage.h"
#include "DebugView++Lib/ProcessInfo.h"
#include "DebugView++Lib/DBWinBuffer.h"
#include "DebugView++Lib/LogSources.h"
#include "DebugView++Lib/LogSource.h"
#include "DebugView++Lib/TestSource.h"
#include "DebugView++Lib/VectorLineBuffer.h"
#include "DebugView++Lib/LogFile.h"
#include "CobaltFusion/scope_guard.h"

namespace fusion {
namespace debugviewpp {

BOOST_AUTO_TEST_SUITE(DebugViewPlusPlusLib)

std::string GetTestString(int i)
{
	return stringbuilder() << "BB_TEST_ABCDEFGHI_EE_" << i;
}

class TestLineBuffer : public LineBuffer
{
public:
	TestLineBuffer(size_t size) : LineBuffer(size)
	{
	}

	virtual ~TestLineBuffer()
	{
	}

	virtual void WaitForReaderTimeout()
	{
		throw std::exception("WaitForReader timeout");
	}
};

BOOST_AUTO_TEST_CASE(HandleTest)
{
	HANDLE rawHandle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
	Win32::Handle handle(rawHandle);
	{ 
		BOOST_REQUIRE_EQUAL(handle.get(), rawHandle);
		Win32::Handle handle1(std::move(handle));
		BOOST_REQUIRE_EQUAL(handle.get(), HANDLE(nullptr));
		BOOST_REQUIRE_EQUAL(handle1.get(), rawHandle);
		{
			Win32::Handle handle2(std::move(handle1));
			handle2.release();
			BOOST_REQUIRE_EQUAL(handle2.get(), HANDLE(nullptr));
		}
		BOOST_REQUIRE_EQUAL(handle1.get(), HANDLE(nullptr));
	}

	{
		Win32::Handle handle1;
		{
			HANDLE zero = nullptr;
			Win32::Handle zeroHandle(zero);
			BOOST_REQUIRE_EQUAL(zeroHandle.get(), HANDLE(nullptr));
			handle1 = std::move(zeroHandle);
			BOOST_REQUIRE_EQUAL(zeroHandle.get(), HANDLE(nullptr));
			BOOST_REQUIRE_EQUAL(handle1.get(), HANDLE(nullptr));
		}
		// this scope ensures having a Handle leave scope that had its guts ripped out by std::move 
		// will not cause nullpointers or exections
	}
}

BOOST_AUTO_TEST_CASE(LineBufferTest1)
{
	TestLineBuffer buffer(64);
	FILETIME ft;
	ft.dwLowDateTime = 42;
	ft.dwHighDateTime = 43;
	buffer.Add(42.0, ft, 0, "test", nullptr);

	auto lines = buffer.GetLines();
	auto line = lines[0];
	BOOST_REQUIRE_EQUAL(line.time, 42.0);
	BOOST_REQUIRE_EQUAL(line.systemTime.dwLowDateTime, 42);
	BOOST_REQUIRE_EQUAL(line.systemTime.dwHighDateTime, 43);
	BOOST_REQUIRE(buffer.Empty());
}

BOOST_AUTO_TEST_CASE(LineBufferTest2)
{
	TestLineBuffer buffer(600);
	Timer timer;

	for (int j=0; j< 1000; ++j)
	{
		//BOOST_MESSAGE("j: " << j << "\n");

		for (int i=0; i<17; ++i)
		{
			//BOOST_MESSAGE("i: " << i << "\n");
			FILETIME ft;
			ft.dwLowDateTime = 43;
			ft.dwHighDateTime = 44;
			buffer.Add(42.0, ft, 0, "test", nullptr);
		}

		auto lines = buffer.GetLines();
		BOOST_REQUIRE_EQUAL(lines.size(), 17);
		for (auto it = lines.begin(); it != lines.end(); ++it)
		{
			auto line = *it;
			BOOST_REQUIRE_EQUAL(line.time, 42.0);
			BOOST_REQUIRE_EQUAL(line.systemTime.dwLowDateTime, 43);
			BOOST_REQUIRE_EQUAL(line.systemTime.dwHighDateTime, 44);
		}
	}
}

BOOST_AUTO_TEST_CASE(IndexedStorageRandomAccess)
{
	using namespace indexedstorage;

	size_t testSize = 10000;
	auto testMax = testSize - 1;

	std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution(0, testMax);
	SnappyStorage s;
	for (size_t i = 0; i < testSize; ++i)
		s.Add(GetTestString(i));

	for (size_t i = 0; i < testSize; ++i)
	{
		size_t j = distribution(generator);  // generates number in the range 0..testMax 
		BOOST_REQUIRE_EQUAL(s[j], GetTestString(j));
	}
}

BOOST_AUTO_TEST_CASE(IndexedStorageCompression)
{
	using namespace indexedstorage;

	// the memory allocator will mess up test results is < 64 kb is allocated
	// make sure SnappyStorage allocates at least ~500kb for reproducable results
	
	// this test is indicative only, on average the SnappyStorage should allocate at most 50% of memory compared to a normal vector.
	// since GetTestString returns an overly simpe-to-compress string, it will appear to perform insanely good.

	size_t testSize = 100000;	
	VectorStorage v;
	SnappyStorage s;

	size_t m0 = ProcessInfo::GetPrivateBytes();

	for (size_t i = 0; i < testSize; ++i)
		v.Add(GetTestString(i));

	size_t m1 = ProcessInfo::GetPrivateBytes();
	size_t usedByVector = m1 - m0;
	BOOST_MESSAGE("VectorStorage requires: " << usedByVector/1024 << " kB");

	for (size_t i = 0; i < testSize; ++i)
		s.Add(GetTestString(i));

	size_t m2 = ProcessInfo::GetPrivateBytes();
	size_t usedBySnappy = m2 - m1;

	BOOST_MESSAGE("SnappyStorage requires: " << usedBySnappy/1024 << " kB (" << (100*usedBySnappy)/usedByVector << "%)");
	BOOST_REQUIRE_GT(0.50*usedByVector, usedBySnappy);
}

// execute as:
// "DebugView++Test.exe" --log_level=test_suite --run_test=*/LogSourcesTest
BOOST_AUTO_TEST_CASE(LogSourcesTest)
{
	LogSources logsources(false);
	auto logsource = logsources.AddTestSource();

	Timer timer;
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 1");
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 2");
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 3");
	BOOST_MESSAGE("3 lines added.");

	auto lines = logsources.GetLines();
	BOOST_MESSAGE("received: " << lines.size() << " lines.");

	BOOST_REQUIRE_EQUAL(lines.size(), 3);

	for (auto it = lines.begin(); it != lines.end(); ++it)
	{
		auto line = *it;
		BOOST_MESSAGE("line: " << line.message);
	}

	const int testsize = 1000;
	BOOST_MESSAGE("Write " << testsize << " lines...");
	for (int i=0; i < testsize; ++i)
	{
		logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "TESTSTRING 1234\n");
	}

	auto morelines = logsources.GetLines();
	BOOST_MESSAGE("received: " << morelines.size() << " lines.");
	BOOST_REQUIRE_EQUAL(morelines.size(), testsize);
}

BOOST_AUTO_TEST_CASE(TimeZoneTest)
{
	Win32::SetPrivilege(SE_TIME_ZONE_NAME, true);
	
	TIME_ZONE_INFORMATION tz;
	GetTimeZoneInformation(&tz);
	auto tzGuard = make_guard([tz] { SetTimeZoneInformation(&tz); });

	TIME_ZONE_INFORMATION testTz = tz;
	testTz.Bias = -480;
	SetTimeZoneInformation(&testTz);

	LogSources logsources(false);
	auto logsource = logsources.AddTestSource();
	//int counter = 0;
	//logsources.SubscribeToUpdate([&counter] () -> bool { counter++; return true; });

	Timer timer;
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 1");
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 2");
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 3");

	auto lines = logsources.GetLines();
	BOOST_REQUIRE_EQUAL(lines.size(), 3);

	LogFile logFile;
	logFile.Add(Message(0.0, Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 1"));
	logFile.Add(Message(0.0, Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 2"));
	logFile.Add(Message(0.0, Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 3"));
	BOOST_REQUIRE_EQUAL(logFile.Count(), 3);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace debugviewpp 
} // namespace fusion
