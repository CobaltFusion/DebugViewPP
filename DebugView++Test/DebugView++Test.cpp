// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"

#define BOOST_TEST_MODULE DebugView++Lib Unit Test

#define _SCL_SECURE_NO_WARNINGS
#include <boost/test/unit_test_gui.hpp>
#include <boost/filesystem.hpp>
#include <random>
#include <fstream>

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
#include "DebugView++Lib/FileIO.h"
#include "DebugView++Lib/Conversions.h"
#include "CobaltFusion/scope_guard.h"
#undef _SCL_SECURE_NO_WARNINGS

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


class ScopedTimezoneBias
{
public:
	ScopedTimezoneBias(LONG Bias)
	{
		Win32::SetPrivilege(SE_TIME_ZONE_NAME, true);
		GetTimeZoneInformation(&m_tz);
		TIME_ZONE_INFORMATION tz = m_tz;
		tz.Bias = Bias;
		SetTimeZoneInformation(&tz);
	}

	~ScopedTimezoneBias()
	{
		SetTimeZoneInformation(&m_tz);
		Win32::SetPrivilege(SE_TIME_ZONE_NAME, false);
	}
private:
	TIME_ZONE_INFORMATION m_tz;
};

std::string SaveLogFile(const LogFile& logfile)
{
	std::string filename = "SaveLoadLogFile_unique_test_filename";
	std::ofstream fs;
	OpenLogFile(fs, WStr(filename), OpenMode::Truncate);
	int count = logfile.Count();
	for (int i = 0; i < count; ++i)
	{
		auto msg = logfile[i];
		WriteLogFileMessage(fs, msg.time, msg.systemTime, msg.processId, msg.processName, msg.text);
	}
	fs.close();
	return filename;
}

LogFile LoadLogFile(const std::string& filename)
{
	// load file test
	std::ifstream file(filename);
	Line line;
	line.processName = "process";
	line.systemTime = Win32::GetSystemTimeAsFileTime();
	LogFile result;
	ReadLogFileMessage(file, line);		// ignore header
	while (ReadLogFileMessage(file, line))
		result.Add(Message(line.time, line.systemTime, line.pid, line.processName, line.message));
	return result;
}

bool AreEqual(const Message& a, const Message& b)
{
	if (std::fabs(a.time - b.time) > 0.000001) return false;
	if (GetTimeText(a.systemTime) != GetTimeText(b.systemTime)) return false;
	if (a.processId != b.processId) return false;
	if (a.processName != b.processName) return false;
	if (a.text != b.text) return false;
	//if (a.color != b.color) return false;	// not stored in file
	return true;
}

bool AreEqual(const LogFile& a, const LogFile& b)
{
	if (a.Count() != b.Count())
		return false;
	int count = a.Count();
	for (int i = 0; i < count; ++i)
	{
		if (!AreEqual(a[i], b[i]))
			return false;
	}
	return true;
}

BOOST_AUTO_TEST_CASE(TimeZone)
{
	Timer timer;
	LogFile logFile;
	auto t1 = Win32::GetSystemTimeAsFileTime();
	logFile.Add(Message(timer.Get(), t1, 0, "processname", "test message 1"));
	auto t2 = Win32::GetSystemTimeAsFileTime();
	logFile.Add(Message(timer.Get(), t2, 0, "processname", "test message 2"));
	auto t3 = Win32::GetSystemTimeAsFileTime();
	logFile.Add(Message(timer.Get(), t3, 0, "processname", "test message 3"));
	auto t4 = Win32::GetSystemTimeAsFileTime();
	logFile.Add(Message(0, t4, 0, "processname", "message 4 (zero time message)"));

	BOOST_TEST_MESSAGE("UTC");
	{
		ScopedTimezoneBias(0);
		auto result = LoadLogFile(SaveLogFile(logFile));
		BOOST_REQUIRE_EQUAL(AreEqual(result, logFile), true);
	}
	BOOST_TEST_MESSAGE("(UTC-8) Fremond");
	{
		ScopedTimezoneBias(-480);
		auto result = LoadLogFile(SaveLogFile(logFile));
		BOOST_REQUIRE_EQUAL(AreEqual(result, logFile), true);
	}
	BOOST_TEST_MESSAGE("(UTC+8:30) Pyongyang Standard Time");
	{
		ScopedTimezoneBias(+510);
		auto result = LoadLogFile(SaveLogFile(logFile));
		BOOST_REQUIRE_EQUAL(AreEqual(result, logFile), true);
	}
	BOOST_TEST_MESSAGE("test boundry case (UTC-12) Baker- / Howland Island (uninhabited islands belonging to the United States)");
	{
		ScopedTimezoneBias(-720);
		auto result = LoadLogFile(SaveLogFile(logFile));
		BOOST_REQUIRE_EQUAL(AreEqual(result, logFile), true);
	}
	BOOST_TEST_MESSAGE("test boundry case (UTC+14) Kiritimati (Christmas Island, also uninhabited, part of the Kiribati Line Islands)");
	{
		ScopedTimezoneBias(+720);
		auto result = LoadLogFile(SaveLogFile(logFile));
		BOOST_REQUIRE_EQUAL(AreEqual(result, logFile), true);
	}

	// test crossing timezones 
	BOOST_TEST_MESSAGE("test sending a logfile into the past (UTC -> UTC-8)");
	{
		auto filename = SaveLogFile(logFile);
		ScopedTimezoneBias(-480);
		auto result = LoadLogFile(filename);
		BOOST_REQUIRE_EQUAL(AreEqual(result, logFile), true);
	}

	// test crossing timezones 
	BOOST_TEST_MESSAGE("test sending a logfile into the future (UTC-8 -> UTC)");
	{
		std::string filename;
		{
			ScopedTimezoneBias(-480);
			filename = SaveLogFile(logFile);
		}
		auto result = LoadLogFile(filename);
		BOOST_REQUIRE_EQUAL(AreEqual(result, logFile), true);
	}

}

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
		//BOOST_TEST_MESSAGE("j: " << j << "\n");

		for (int i=0; i<17; ++i)
		{
			//BOOST_TEST_MESSAGE("i: " << i << "\n");
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
	BOOST_TEST_MESSAGE("VectorStorage requires: " << usedByVector/1024 << " kB");

	for (size_t i = 0; i < testSize; ++i)
		s.Add(GetTestString(i));

	size_t m2 = ProcessInfo::GetPrivateBytes();
	size_t usedBySnappy = m2 - m1;

	BOOST_TEST_MESSAGE("SnappyStorage requires: " << usedBySnappy/1024 << " kB (" << (100*usedBySnappy)/usedByVector << "%)");
	BOOST_REQUIRE_GT(0.50*usedByVector, usedBySnappy);
}

// execute as:
// "DebugView++Test.exe" --log_level=test_suite --run_test=*/LogSourcesReceiveMessages
BOOST_AUTO_TEST_CASE(LogSourcesReceiveMessages)
{
	LogSources logsources(false);
	auto logsource = logsources.AddTestSource();

	Timer timer;
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 1");
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 2");
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 3");
	BOOST_TEST_MESSAGE("3 lines added.");

	auto lines = logsources.GetLines();
	BOOST_TEST_MESSAGE("received: " << lines.size() << " lines.");

	BOOST_REQUIRE_EQUAL(lines.size(), 3);

	for (auto it = lines.begin(); it != lines.end(); ++it)
	{
		auto line = *it;
		BOOST_TEST_MESSAGE("line: " << line.message);
	}

	const int testsize = 1000;
	BOOST_TEST_MESSAGE("Write " << testsize << " lines...");
	for (int i=0; i < testsize; ++i)
	{
		logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "TESTSTRING 1234\n");
	}

	auto morelines = logsources.GetLines();
	BOOST_TEST_MESSAGE("received: " << morelines.size() << " lines.");
	BOOST_REQUIRE_EQUAL(morelines.size(), testsize);
}

BOOST_AUTO_TEST_CASE(LogSourcesCharacterPreservation)
{
	LogSources logsources(false);
	auto logsource = logsources.AddTestSource();

	Timer timer;
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "TrailingSpace ");
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "TrailingTab\t");

	auto lines = logsources.GetLines();
	BOOST_REQUIRE_EQUAL(lines.size(), 2);
	BOOST_REQUIRE_EQUAL(lines[0].message, "TrailingSpace ");	// space preserved
	BOOST_REQUIRE_EQUAL(lines[1].message, "TrailingTab\t");		// tab preserved
}

BOOST_AUTO_TEST_CASE(LogSourcesTabHandling)
{
	LogSources logsources(false);
	auto logsource = logsources.AddTestSource();

	Timer timer;
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "\tTabPrefix");
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "\t\tTwoTabsPrefixed");

	auto lines = logsources.GetLines();
	BOOST_REQUIRE_EQUAL(lines.size(), 2);
	BOOST_REQUIRE_EQUAL(lines[0].message, "\tTabPrefix");	// space preserved
	BOOST_REQUIRE_EQUAL(lines[1].message, "\t\tTwoTabsPrefixed");	// space preserved
}

BOOST_AUTO_TEST_CASE(LogSourcesNewLineHandling)
{
	LogSources logsources(false);
	auto logsource = logsources.AddTestSource();

	Timer timer;
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "NewLinePostfix\n");
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "NewLineCarriageReturnPostfix\n\r");
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "CarriageReturnNewLinePostfix\r\n");

	auto lines = logsources.GetLines();
	BOOST_REQUIRE_EQUAL(lines.size(), 3);
	BOOST_REQUIRE_EQUAL(lines[0].message, "NewLinePostfix");
	BOOST_REQUIRE_EQUAL(lines[1].message, "NewLineCarriageReturnPostfix");
	BOOST_REQUIRE_EQUAL(lines[2].message, "CarriageReturnNewLinePostfix");
}

std::wstring GetExecutePath()
{
	using namespace boost;
	auto path = filesystem::system_complete(filesystem::path( Win32::GetCommandLineArguments()[0]));
	return path.remove_filename().c_str();
}

BOOST_AUTO_TEST_CASE(LogSourceDbwinReader)
{
	std::string dbgMsgSrc = stringbuilder() << GetExecutePath() << "\\DbgMsgSrc.exe";
	BOOST_REQUIRE_EQUAL(FileExists(dbgMsgSrc.c_str()), true);
	std::string cmd = stringbuilder() << "start \"\" " << dbgMsgSrc << " ";

	LogSources logsources(true);
	logsources.AddDBWinReader(false);
	logsources.SetAutoNewLine(true);

	BOOST_TEST_MESSAGE("cmd: " << cmd);
	system((cmd + "-n").c_str());
	BOOST_TEST_MESSAGE("done.");
	//	 logsources.Abort(); // hangs? 

	Sleep(200);

	auto lines = logsources.GetLines();
	BOOST_REQUIRE_EQUAL(lines.size(), 1);
}

// add test simulating MFC application behaviour (pressing pause/unpause lots of times during significant incomming messages)

BOOST_AUTO_TEST_SUITE_END()

} // namespace debugviewpp 
} // namespace fusion
