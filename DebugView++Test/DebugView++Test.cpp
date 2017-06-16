// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"

#define BOOST_TEST_MODULE DebugView++Lib Unit Test

#define _SCL_SECURE_NO_WARNINGS
#include <boost/test/unit_test_gui.hpp>

#include <filesystem>
#include <random>
#include <fstream>
#include <iostream>

#include "Win32/Utilities.h"
#include "Win32/Win32Lib.h"
#include "CobaltFusion/stringbuilder.h"
#include "CobaltFusion/ExecutorClient.h"
#include "CobaltFusion/Executor.h"
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

std::string GetTestString(size_t i)
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

std::string GetTestFileName()
{
	using namespace std::experimental::filesystem;
	return absolute(path("SaveLoadLogFile_unique_test_filename")).string();
}

std::string SaveLogFile(const LogFile& logfile)
{
	auto filename = GetTestFileName();
	std::ofstream fs;
	OpenLogFile(fs, WStr(filename), OpenMode::Truncate);
	size_t count = logfile.Count();
	for (size_t i = 0; i < count; ++i)
	{
		auto msg = logfile[i];
		WriteLogFileMessage(fs, msg.time, msg.systemTime, msg.processId, msg.processName, msg.text);
	}
	fs.close();
	return filename;
}

std::string AppendLogFile(const LogFile& logfile)
{
	auto filename = GetTestFileName();
	std::ofstream fs;
	OpenLogFile(fs, WStr(filename), OpenMode::Append);
	size_t count = logfile.Count();
	for (size_t i = 0; i < count; ++i)
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
	size_t count = a.Count();
	for (size_t i = 0; i < count; ++i)
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
		BOOST_TEST(AreEqual(result, logFile));
	}
	BOOST_TEST_MESSAGE("(UTC-8) Fremond");
	{
		ScopedTimezoneBias(-480);
		auto result = LoadLogFile(SaveLogFile(logFile));
		BOOST_TEST(AreEqual(result, logFile));
	}
	BOOST_TEST_MESSAGE("(UTC+8:30) Pyongyang Standard Time");
	{
		ScopedTimezoneBias(+510);
		auto result = LoadLogFile(SaveLogFile(logFile));
		BOOST_TEST(AreEqual(result, logFile));
	}
	BOOST_TEST_MESSAGE("test boundry case (UTC-12) Baker- / Howland Island (uninhabited islands belonging to the United States)");
	{
		ScopedTimezoneBias(-720);
		auto result = LoadLogFile(SaveLogFile(logFile));
		BOOST_TEST(AreEqual(result, logFile));
	}
	BOOST_TEST_MESSAGE("test boundry case (UTC+14) Kiritimati (Christmas Island, also uninhabited, part of the Kiribati Line Islands)");
	{
		ScopedTimezoneBias(+720);
		auto result = LoadLogFile(SaveLogFile(logFile));
		BOOST_TEST(AreEqual(result, logFile));
	}

	// test crossing timezones 
	BOOST_TEST_MESSAGE("test sending a logfile into the past (UTC -> UTC-8)");
	{
		auto filename = SaveLogFile(logFile);
		ScopedTimezoneBias(-480);
		auto result = LoadLogFile(filename);
		BOOST_TEST(AreEqual(result, logFile));
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
		BOOST_TEST(AreEqual(result, logFile));
	}
}

BOOST_AUTO_TEST_CASE(HandleTest)
{
	static constexpr HANDLE nullHandle = nullptr;

	HANDLE rawHandle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
	Win32::Handle handle(rawHandle);
	{ 
		BOOST_TEST(handle.get() == rawHandle);
		Win32::Handle handle1(std::move(handle));
		BOOST_TEST(handle.get() == nullHandle);
		BOOST_TEST(handle1.get() == rawHandle);
		{
			Win32::Handle handle2(std::move(handle1));
			handle2.release();
			BOOST_TEST(handle2.get() == nullHandle);
		}
		BOOST_TEST(handle1.get() == nullHandle);
	}

	{
		Win32::Handle handle1;
		{
			HANDLE zero = nullptr;
			Win32::Handle zeroHandle(zero);
			BOOST_TEST(zeroHandle.get() == nullHandle);
			handle1 = std::move(zeroHandle);
			BOOST_TEST(zeroHandle.get() == nullHandle);
			BOOST_TEST(handle1.get() == nullHandle);
		}
		// this scope ensures having a Handle leave scope that had its guts ripped out by std::move 
		// will not cause nullpointers or exeptions
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
	BOOST_TEST(line.time == 42.0);
	BOOST_TEST(line.systemTime.dwLowDateTime == 42);
	BOOST_TEST(line.systemTime.dwHighDateTime == 43);
	BOOST_TEST(buffer.Empty());
}

BOOST_AUTO_TEST_CASE(LineBufferTest2)	// test overflows boosttestui with log-output locking its GUI up temporarily
{
	TestLineBuffer buffer(600);
	Timer timer;
	std::cout << "LineBufferTest2 running..." << std::endl;  
	for (int j = 0; j < 100; ++j)
	{
		//BOOST_TEST_MESSAGE("j: " << j << "\n");

		for (int i = 0; i < 17; ++i)
		{
			//BOOST_TEST_MESSAGE("i: " << i << "\n");
			FILETIME ft;
			ft.dwLowDateTime = 43;
			ft.dwHighDateTime = 44;
			buffer.Add(42.0, ft, 0, "test", nullptr);
		}

		auto lines = buffer.GetLines();
		BOOST_TEST(lines.size() == 17);
		for (auto& line : lines)
		{
			BOOST_TEST(line.time == 42.0);
			BOOST_TEST(line.systemTime.dwLowDateTime == 43);
			BOOST_TEST(line.systemTime.dwHighDateTime == 44);
		}
	}
}

BOOST_AUTO_TEST_CASE(IndexedStorageRandomAccess)
{
	using namespace indexedstorage;

	size_t testSize = 10000;
	auto testMax = testSize - 1;

	std::mt19937 generator;
	std::uniform_int_distribution<size_t> distribution(0, testMax);
	SnappyStorage s;
	for (size_t i = 0; i < testSize; ++i)
		s.Add(GetTestString(i));

	// random access can be somewhat slow in debug-mode, this is expected behaviour
	bool failed = false;
	for (size_t i = 0; i < testSize/10; ++i)
	{
		size_t j = distribution(generator);  // generates number in the range 0..testMax 
		if (s[j] != GetTestString(j))
		{
			failed = true;
			break;
		};
	}
	BOOST_TEST(!failed);
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
	BOOST_TEST(0.50*usedByVector > usedBySnappy);
}

// execute as:
// "DebugView++Test.exe" --log_level=test_suite --run_test=*/LogSourcesReceiveMessages
BOOST_AUTO_TEST_CASE(LogSourcesReceiveMessages)
{
	ActiveExecutorClient executor;
	LogSources logsources(executor, false);
	LogSource* logsource;
	executor.Call([&] { logsource = logsources.AddTestSource();  });

	Timer timer;
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 1");
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 2");
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 3");
	BOOST_TEST_MESSAGE("3 lines added.");

	Lines lines;
	executor.Call([&] { lines = logsources.GetLines(); });
	BOOST_TEST_MESSAGE("received: " << lines.size() << " lines.");

	BOOST_TEST(lines.size() == 3);

	for (auto& line : lines)
	{
		BOOST_TEST_MESSAGE("line: " << line.message);
	}

	const int testsize = 1000;
	BOOST_TEST_MESSAGE("Write " << testsize << " lines...");
	for (int i = 0; i < testsize; ++i)
	{
		logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "TESTSTRING 1234\n");
	}

	Lines morelines;
	executor.Call([&] { morelines = logsources.GetLines(); });

	BOOST_TEST_MESSAGE("received: " << morelines.size() << " lines.");
	BOOST_TEST(morelines.size() == testsize);
}

BOOST_AUTO_TEST_CASE(LogSourcesCharacterPreservation)
{
	ActiveExecutorClient executor;
	LogSources logsources(executor, false);
	LogSource* logsource;
	executor.Call([&] { logsource = logsources.AddTestSource();  });

	Timer timer;
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "TrailingSpace ");
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "TrailingTab\t");

	Lines lines;
	executor.Call([&] { lines = logsources.GetLines(); });
	BOOST_TEST(lines.size() == 2);
	BOOST_TEST(lines[0].message == "TrailingSpace ");	// space preserved
	BOOST_TEST(lines[1].message == "TrailingTab\t");	// tab preserved
}

BOOST_AUTO_TEST_CASE(LogSourcesTabHandling)
{
	ActiveExecutorClient executor;
	LogSources logsources(executor, false);
	LogSource* logsource;
	executor.Call([&] { logsource = logsources.AddTestSource();  });

	Timer timer;
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "\tTabPrefix");
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "\t\tTwoTabsPrefixed");

	Lines lines;
	executor.Call([&] { lines = logsources.GetLines(); });
	BOOST_TEST(lines.size() == 2);
	BOOST_TEST(lines[0].message == "\tTabPrefix");	// space preserved
	BOOST_TEST(lines[1].message == "\t\tTwoTabsPrefixed");	// space preserved
}

BOOST_AUTO_TEST_CASE(LogSourcesNewLineHandling)
{
	ActiveExecutorClient executor;
	LogSources logsources(executor, false);
	LogSource* logsource;
	executor.Call([&] { logsource = logsources.AddTestSource();  });

	Timer timer;
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "NewLinePostfix\n");
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "NewLineCarriageReturnPostfix\n\r");
	logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "CarriageReturnNewLinePostfix\r\n");

	Lines lines;
	executor.Call([&] { lines = logsources.GetLines(); });
	BOOST_TEST(lines.size() == 3);
	BOOST_TEST(lines[0].message == "NewLinePostfix");
	BOOST_TEST(lines[1].message == "NewLineCarriageReturnPostfix");
	BOOST_TEST(lines[2].message == "CarriageReturnNewLinePostfix");
}

std::wstring GetExecutePath()
{
	auto path = system_complete(std::experimental::filesystem::path( Win32::GetCommandLineArguments()[0]));
	return path.remove_filename().c_str();
}

BOOST_AUTO_TEST_CASE(LogSourceLoopback)
{
	using namespace std::chrono_literals;

	std::string dbgMsgSrc = stringbuilder() << GetExecutePath() << "\\DbgMsgSrc.exe";
	BOOST_TEST(FileExists(dbgMsgSrc.c_str()));
	std::string cmd = stringbuilder() << "start \"\" " << dbgMsgSrc << " ";

	Lines lines;
	auto executor = std::make_unique<ActiveExecutorClient>();
	{
		LogSources logsources(*executor, true);
		executor->Call([&] { logsources.AddDBWinReader(false); });
		executor->Call([&] { logsources.SetAutoNewLine(true); });

		executor->Call([&] { logsources.AddMessage("Test message 1"); });
		executor->Call([&] { logsources.AddMessage("Test message 2"); });
		std::this_thread::sleep_for(200ms);
		executor->Call([&] { logsources.Abort(); });

		executor->Call([&] { lines = logsources.GetLines(); });
	}
	executor.reset();

	BOOST_TEST(lines.size() == 2);
}

BOOST_AUTO_TEST_CASE(LogSourceDBWinReader)
{
	using namespace std::chrono_literals;

	std::string dbgMsgSrc = stringbuilder() << GetExecutePath() << "\\DbgMsgSrc.exe";
	BOOST_TEST(FileExists(dbgMsgSrc.c_str()));
	std::string cmd = stringbuilder() << "start \"\" " << dbgMsgSrc << " ";

	auto executor = std::make_unique<ActiveExecutorClient>();
	Lines lines;
	{
		LogSources logsources(*executor, true);
		executor->Call([&] { logsources.SetAutoNewLine(true); });
		executor->Call([&] { logsources.AddDBWinReader(false); });

		BOOST_TEST_MESSAGE("cmd: " << cmd);
		system((cmd + "-n").c_str());
		BOOST_TEST_MESSAGE("done.");
		std::this_thread::sleep_for(200ms);
		executor->Call([&] { logsources.Abort(); });
		executor->Call([&] { lines = logsources.GetLines(); });
	}
	executor.reset();

	BOOST_TEST(lines.size() == 1);
}

std::string CreateTestFile()
{
	Timer timer;
	LogFile logFile;
	logFile.Add(Message(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "test message 1"));
	logFile.Add(Message(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "test message 2"));
	logFile.Add(Message(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "test message 3"));
	logFile.Add(Message(0, Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 4 (zero time message)"));
	return SaveLogFile(logFile);
}

std::string AppendToTestFile()
{
	Timer timer;
	LogFile logFile;
	logFile.Add(Message(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "test message appended"));
	return AppendLogFile(logFile);
}

std::vector<size_t> GetNewLineOffsets(const std::string& filename)
{
	std::vector<size_t> result;
	std::ifstream file(filename);
	Line line;
	while (ReadLogFileMessage(file, line))
		result.push_back(static_cast<size_t>(file.tellg()));
	return result;
}

void RemoveLinesFromFile(const std::string& filename, int linesToRemove)
{
	auto offsets = GetNewLineOffsets(filename);
	auto offset = offsets[offsets.size() - 1 - linesToRemove];
	Win32::HFile(GetTestFileName()).resize(offset);
}

BOOST_AUTO_TEST_CASE(LogSourceAnyFileReader)
{
	using namespace std::chrono_literals;
	auto executor = std::make_unique<ActiveExecutorClient>();
	LogSources logsources(*executor, true);
	auto filename = CreateTestFile();
	executor->Call([&] { logsources.SetAutoNewLine(true); });
	executor->Call([&] { logsources.AddAnyFileReader(WStr(filename), true); });
	std::this_thread::sleep_for(200ms);

	Lines lines;
	executor->Call([&] { lines = logsources.GetLines(); });
	BOOST_TEST(lines.size() == 5);
}

BOOST_AUTO_TEST_CASE(LogSourceAnyFileReaderResychronizeShrinkingFile)
{
	using namespace std::chrono_literals;
	auto executor = std::make_unique<ActiveExecutorClient>();
	LogSources logsources(*executor, true);
	auto filename = CreateTestFile();
	executor->Call([&] { logsources.SetAutoNewLine(true); });
	executor->Call([&] { logsources.AddAnyFileReader(WStr(filename), true); });
	std::this_thread::sleep_for(200ms);

	Lines lines;
	executor->Call([&] { lines = logsources.GetLines(); });
	BOOST_TEST(lines.size() == 5);

	RemoveLinesFromFile(GetTestFileName(), 2);
	AppendToTestFile();
	AppendToTestFile();
	AppendToTestFile();

	std::this_thread::sleep_for(200ms);
	executor->Call([&] { lines = logsources.GetLines(); });
	BOOST_TEST(lines.size() == 4);
}

std::string GetTestFileAsString()
{
	std::ifstream file(GetTestFileName());
	std::stringstream ss;
	ss << file.rdbuf();
	return ss.str();
}

BOOST_AUTO_TEST_CASE(LogSourceAnyFileReaderRewriteByteByByte)
{
	using namespace std::chrono_literals;
	auto executor = std::make_unique<ActiveExecutorClient>();
	LogSources logsources(*executor, true);
	auto filename = CreateTestFile();
	executor->Call([&] { logsources.SetAutoNewLine(true); });
	executor->Call([&] { logsources.AddAnyFileReader(WStr(filename), true); });
	std::this_thread::sleep_for(200ms);

	{
		Lines lines;
		executor->Call([&] { lines = logsources.GetLines(); });
		BOOST_TEST(lines.size() == 5);
	}
	auto content = GetTestFileAsString();

	std::ofstream fs;
	fs.open(GetTestFileName(), std::ofstream::trunc);
	fs.close();
	std::this_thread::sleep_for(100ms);
	for (char c : content)
	{
		std::ofstream fs;
		fs.open(GetTestFileName(), std::ofstream::app);
		fs << c;
		// just flushing the fstream doesn't force a trigger of Find....ChangeNotification, but closing the fstream does the trick
		fs.flush();
		fs.close();
		Sleep(20);
	}

	std::this_thread::sleep_for(200ms);
	Lines lines;
	executor->Call([&] { lines = logsources.GetLines(); });
	for (auto& line : lines)
	{
		BOOST_TEST(line.message.find("xception") == std::string::npos);
		//std::cout << line.message << std::endl;
	}
	BOOST_TEST(lines.size() == 6);
	BOOST_TEST(lines.at(0).message.back() == '0'); // line ends in '...offset 0' 
	BOOST_TEST(lines.at(1).message == "File Identification Header, DebugView++ Format Version 1");
	BOOST_TEST(lines.at(2).message == "test message 1");
	BOOST_TEST(lines.at(3).message == "test message 2");
	BOOST_TEST(lines.at(4).message == "test message 3");
	BOOST_TEST(lines.at(5).message == "message 4 (zero time message)");
}

std::string CreateAsciiTestFile()
{
	auto filename = GetTestFileName();
	std::ofstream fs;
	fs.open(filename, std::ofstream::trunc);
	fs << "This is the first line\n";
	fs << "\n"; // empty line
	fs << "This is the last line\n";
	return filename;
}

BOOST_AUTO_TEST_CASE(LogSourceAnyFileReaderEmptyLines)
{
	using namespace std::chrono_literals;
	auto executor = std::make_unique<ActiveExecutorClient>();
	LogSources logsources(*executor, true);
	auto filename = CreateAsciiTestFile();
	executor->Call([&] { logsources.SetAutoNewLine(true); });
	executor->Call([&] { logsources.AddAnyFileReader(WStr(filename), true); });
	std::this_thread::sleep_for(200ms);

	{
		Lines lines;
		executor->Call([&] { lines = logsources.GetLines(); });
		BOOST_TEST(lines.size() == 4);
		BOOST_TEST(lines.at(0).message.find("tailing") != std::string::npos);
		BOOST_TEST(lines.at(1).message.find("first") != std::string::npos);
		BOOST_TEST(lines.at(2).message == "", "The expected newline is gone!");
		BOOST_TEST(lines.at(3).message.find("last") != std::string::npos);
		for (auto& line : lines)
		{
			BOOST_TEST(line.message.find("xception") == std::string::npos);
			std::cout << line.message << std::endl;
		}
	}
}

BOOST_AUTO_TEST_CASE(LogSourceLoopbackOrdering)
{
	using namespace std::chrono_literals;
	auto executor = std::make_unique<ActiveExecutorClient>();
	LogSources logsources(*executor, true);
	auto filename = CreateAsciiTestFile();
	executor->Call([&] { logsources.SetAutoNewLine(true); });
	executor->Call([&] { logsources.AddMessage("Loopback message 1"); });
	executor->Call([&] { logsources.AddMessage("Loopback message 2"); });
	executor->Call([&] { logsources.AddMessage("Loopback message 3"); });
	executor->Call([&] { logsources.AddAnyFileReader(WStr(filename), true); });
	std::this_thread::sleep_for(200ms);
	Lines lines;
	executor->Call([&] { lines = logsources.GetLines(); });

	BOOST_TEST(lines.at(0).message == "Loopback message 1");
	BOOST_TEST(lines.at(1).message == "Loopback message 2");
	BOOST_TEST(lines.at(2).message == "Loopback message 3");
	BOOST_TEST(lines.at(3).message.find("tailing") != std::string::npos);
	BOOST_TEST(lines.at(4).message.find("first") != std::string::npos);
	BOOST_TEST(lines.at(5).message == "", "The expected newline is gone!");
	BOOST_TEST(lines.at(6).message.find("last") != std::string::npos);
	for (auto& line : lines)
	{
		std::cout << line.message << std::endl;
	}
}

std::string CreateUTF16LETestFile(int linecount)
{
	auto filename = GetTestFileName();
	std::wfstream fs;
	fs.imbue(std::locale(fs.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>));
	fs.open(filename, std::ios::binary | std::ios::trunc | std::ios::in | std::ios::out);
	for (int i = 0; i < linecount; ++i)
	{
		fs << "This is line number [" << i + 1 << "]\n";
	}
	fs.close();
	return filename;
}

BOOST_AUTO_TEST_CASE(LoadUTF16LE)
{
	using namespace std::chrono_literals;
	auto executor = std::make_unique<ActiveExecutorClient>();
	LogSources logsources(*executor, true);
	int lineCount = 65000;
	auto filename = CreateUTF16LETestFile(lineCount);
	executor->Call([&] { logsources.SetAutoNewLine(true); });
	executor->Call([&] { logsources.AddAnyFileReader(WStr(filename), true); });

	// assumption: should be done in 4 seconds, even in debug mode
	int totalLines = 0; 
	for (int i=0; i<20;++i)
	{
		std::this_thread::sleep_for(200ms);
		Lines lines;
		executor->Call([&] { lines = logsources.GetLines(); });
		totalLines += lines.size();
	}

	// (lineCount + 1) because the first line looks like: "Started tailing ....txt identified as '...'
	BOOST_TEST(totalLines == (lineCount + 1));
}

// add test simulating MFC application behaviour (pressing pause/unpause lots of times during significant incomming messages)

BOOST_AUTO_TEST_SUITE_END()

} // namespace debugviewpp 
} // namespace fusion
