// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <iostream>
#include <vector>
#include <memory>
#include "Win32Lib/utilities.h"
#include "DebugView++Lib/DBWinReader.h"
#include "DebugView++Lib/ProcessInfo.h"
#include "DebugView++Lib/LogSources.h"
#include "DebugView++Lib/LineBuffer.h"
#include "../DebugView++/version.h"
#include <boost/asio.hpp> 
#include <boost/algorithm/string.hpp>

namespace fusion {
namespace debugviewpp {

struct Settings
{
	bool timestamp;
	bool performanceCounter;
	bool tabs;
	bool pid;
	bool processName;
	bool autonewline;
	bool flush;
};

std::string GetTimeText(double time)
{
	return stringbuilder() << std::fixed << std::setprecision(6) << time;
}

std::string GetTimeText(const SYSTEMTIME& st)
{
	char buf[32];
	sprintf_s(buf, "%d:%02d:%02d.%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	return buf;
}

std::string GetTimeText(const FILETIME& ft)
{
	return GetTimeText(FileTimeToSystemTime(FileTimeToLocalFileTime(ft)));
}

void OutputDetails(Settings settings, const Line& line)
{
	std::string separator = settings.tabs ? "\t" : " ";
	if (settings.timestamp) 
	{
		std::cout << GetTimeText(line.systemTime) << separator;
	}
	if (settings.performanceCounter) 
	{
		std::cout << GetTimeText(line.time) << separator;
	}
	if (settings.pid) 
	{
		std::cout << line.pid << separator;
	}
	if (settings.processName) 
	{
		std::cout << line.processName.c_str() << separator;
	}
}

void ShowMessages(Settings settings)
{
	LogSources sources(true);
	sources.AddDBWinReader(false);
	if (HasGlobalDBWinReaderRights())
		sources.AddDBWinReader(true);

	sources.SetAutoNewLine(settings.autonewline);

	std::string separator = settings.tabs ? "\t" : " ";
	for (;;)
	{
		auto lines = sources.GetLines();
		for (auto i=lines.begin(); i != lines.end(); ++i)
		{
			OutputDetails(settings, *i);
			std::cout << separator << i->message.c_str() << "\n";
		}
		if (settings.flush)
		{
			std::cout.flush();
		}
		Sleep(100);
	}
}

} // namespace debugviewpp
} // namespace fusion

#include <algorithm>

char* getCmdOption(char ** begin, char ** end, const std::string& option)
{
	char ** itr = std::find(begin, end, option);
	if (itr != end && ++itr != end)
	{
		return *itr;
	}
	return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option)
{
	return std::find(begin, end, option) != end;
}

int main(int argc, char* argv[])
try
{
	using namespace fusion::debugviewpp;

	Settings settings = {0};
	std::cout << "DebugViewConsole v" << VERSION_STR << std::endl;
	settings.timestamp = false;
	if (cmdOptionExists(argv, argv+argc, "-h") || cmdOptionExists(argv, argv+argc, "--help"))
	{
		std::cout << "-h: this help message\n";
		std::cout << "-u: send a UDP test-message (used only for debugging)\n";
		std::cout << "-s: prefix messages with system time\n";
		std::cout << "-q: prefix message with high-precision (<1us) offset (from QueryPerformanceCounter)\n";
		std::cout << "-t: tab-separated output\n";
		std::cout << "-p: add PID (process ID)\n";
		std::cout << "-n: add process name\n";
		std::cout << "-a: auto-newline (\\n's in the message will split the message into multiple lines)\n";
		std::cout << "-f: flush\n";
		std::cout << "-v: verbose output\n";
		exit(0);
	}

	bool verbose = false;
	if (cmdOptionExists(argv, argv+argc, "-v"))
	{
		if (verbose) std::cout << "-s: verbose output\n";
		verbose = true;
	}

	if (cmdOptionExists(argv, argv+argc, "-s"))
	{
		if (verbose) std::cout << "-s: including systemtime\n";
		settings.timestamp = true;
	}
	if (cmdOptionExists(argv, argv+argc, "-q"))
	{
		if (verbose) std::cout << "-q: including relative high-precision offset\n";
		settings.performanceCounter = true;
	}
	if (cmdOptionExists(argv, argv+argc, "-t"))
	{
		if (verbose) std::cout << "-t: output tab-separated output\n";
		settings.tabs = true;
	}
	if (cmdOptionExists(argv, argv+argc, "-p"))
	{
		if (verbose) std::cout << "-p: add PID (process ID)\n";
		settings.pid = true;
	}
	if (cmdOptionExists(argv, argv+argc, "-n"))
	{
		if (verbose) std::cout << "-n: add process name\n";
		settings.processName = true;
	}
	if (cmdOptionExists(argv, argv+argc, "-a"))
	{
		if (verbose) std::cout << "-a: auto-newline (\\n's in the message will split the message into multiple lines)\n";
		settings.autonewline = true;
	}
	if (cmdOptionExists(argv, argv+argc, "-f"))
	{
		if (verbose) std::cout << "-f: auto flush (write to disk more often)\n";
		settings.flush = true;
	}

	if (cmdOptionExists(argv, argv+argc, "-u"))
	{
		if (verbose) std::cout << "Send UDP test message...\n";
		using namespace boost::asio::ip;
		boost::asio::io_service io_service;
		udp::resolver resolver(io_service);
		udp::resolver::query query(udp::v4(), "255.255.255.255", "2999");
		udp::endpoint receiver_endpoint = *resolver.resolve(query);
		udp::socket socket(io_service);
		socket.open(udp::v4());

		// enable broadcast
		boost::asio::socket_base::broadcast option(true);
		socket.set_option(option);

		std::string msg = argv[1];
		std::cout << msg << std::endl;
		socket.send_to(boost::asio::buffer(msg), receiver_endpoint);
	}
	else
	{
		std::cout << "Listining for OutputDebugString messages..." << std::endl;
		ShowMessages(settings);
	}
	return 0;
}
catch (std::exception& e)
{
	std::cerr << "Unexpected error occurred: " << e.what() << std::endl;
	std::string message(e.what());
	if (message.find("CreateDBWinBufferMapping") != std::string::npos)
	{
		std::cerr << "Another DebugView++ (or similar application) might be running. " << std::endl;
	}
	return 1;
}
