// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <memory>
#include <algorithm>
#include <boost/asio.hpp> 
#include <boost/algorithm/string.hpp>
#include "Win32/Utilities.h"
#include "CobaltFusion/scope_guard.h"
#include "CobaltFusion/Str.h"
#include "DebugView++Lib/DBWinBuffer.h"
#include "DebugView++Lib/DBWinReader.h"
#include "DebugView++Lib/FileIO.h"
#include "DebugView++Lib/ProcessInfo.h"
#include "DebugView++Lib/LogSources.h"
#include "DebugView++Lib/Conversions.h"
#include "DebugView++Lib/LineBuffer.h"
#include "../DebugView++/version.h"

namespace fusion
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
	bool linenumber;
	bool console;
	std::string filename;
};

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

static bool g_quit = false;

void Quit()
{
	g_quit = true;
}

void LogMessages(Settings settings)
{
	using namespace std::chrono_literals;

	LogSources sources(true);
	sources.AddDBWinReader(false);
	if (HasGlobalDBWinReaderRights())
		sources.AddDBWinReader(true);

	sources.SetAutoNewLine(settings.autonewline);

	std::ofstream fs;

	if (!settings.filename.empty())
	{
		OpenLogFile(fs, WStr(settings.filename));
		fs.flush();
	}

	auto guard = make_guard([&fs, &settings]()
	{
		if (!settings.filename.empty())
		{
			fs.flush();
			fs.close();
			std::cout << "Log file closed.\n";
		}
	});

	std::string separator = settings.tabs ? "\t" : " ";
	while (!g_quit)
	{
		auto lines = sources.GetLines();
		int linenumber = 0;
		for (auto& line : lines)
		{
			if (settings.console)
			{
				if (settings.linenumber)
				{
					++linenumber;
					std::cout << std::setw(5) << std::setfill('0') << linenumber << std::setfill(' ') << separator;
				}
				OutputDetails(settings, line);
				std::cout << separator << line.message.c_str() << "\n";
			}
			if (!settings.filename.empty())
			{
				WriteLogFileMessage(fs, line.time, line.systemTime, line.pid, line.processName, line.message);
			}
		}
		if (settings.flush)
		{
			std::cout.flush();
			fs.flush();
		}
		std::this_thread::sleep_for(250ms);
	}
	std::cout.flush();
}

} // namespace debugviewpp
} // namespace fusion

char* getCmdOption(char** begin, char** end, const std::string& option)
{
	char** itr = std::find(begin, end, option);
	return itr != end && ++itr != end ? *itr : nullptr;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option)
{
	return std::find(begin, end, option) != end;
}

BOOL WINAPI ConsoleHandler(DWORD dwType)
{
	switch (dwType)
	{
	case CTRL_C_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		fusion::debugviewpp::Quit();
		// report as handled, so no other handler gets called.
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

int main(int argc, char* argv[])
try
{
	using namespace fusion::debugviewpp;

	// install CTRL-C handler
	SetConsoleCtrlHandler(ConsoleHandler, TRUE);

	Settings settings = {0};
	std::cout << "DebugViewConsole v" << VERSION_STR << std::endl;
	settings.timestamp = false;
	if (cmdOptionExists(argv, argv+argc, "-h") || cmdOptionExists(argv, argv+argc, "--help"))
	{
		std::cout << "options:\n";
		std::cout << "  -h: this help message\n";
		std::cout << "  -a: auto-newline (automatically adds newline to messages without a newline, most people want this on)\n";
		std::cout << "  -f: flush (aggressively flush buffers, if unsure, do not use)\n";
		std::cout << "  -v: verbose output\n";
		std::cout << "  -d <file>: write to .dblog file\n";
		std::cout << "  -c enable console output\n";
		std::cout << "console output options: (do not effect the dblog file)\n";
		//std::cout << "-u: send a UDP test-message (used only for debugging)\n";
		std::cout << "  -l: prefix line number\n";
		std::cout << "  -s: prefix messages with system time\n";
		std::cout << "  -q: prefix message with high-precision (<1us) offset (from QueryPerformanceCounter)\n";
		std::cout << "  -t: tab-separated output\n";
		std::cout << "  -p: add PID (process ID)\n";
		std::cout << "  -n: add process name\n";
		exit(0);
	}

	bool verbose = false;
	if (cmdOptionExists(argv, argv + argc, "-v"))
	{
		if (verbose) std::cout << "-s: verbose output\n";
		verbose = true;
	}

	if (cmdOptionExists(argv, argv + argc, "-l"))
	{
		if (verbose)
			std::cout << "-l: prefix line number\n";
		settings.linenumber = true;
	}

	if (cmdOptionExists(argv, argv + argc, "-s"))
	{
		if (verbose)
			std::cout << "-s: including systemtime\n";
		settings.timestamp = true;
	}
	if (cmdOptionExists(argv, argv + argc, "-q"))
	{
		if (verbose)
			std::cout << "-q: including relative high-precision offset\n";
		settings.performanceCounter = true;
	}
	if (cmdOptionExists(argv, argv + argc, "-t"))
	{
		if (verbose)
			std::cout << "-t: output tab-separated output\n";
		settings.tabs = true;
	}
	if (cmdOptionExists(argv, argv + argc, "-p"))
	{
		if (verbose)
			std::cout << "-p: add PID (process ID)\n";
		settings.pid = true;
	}
	if (cmdOptionExists(argv, argv + argc, "-n"))
	{
		if (verbose)
			std::cout << "-n: add process name\n";
		settings.processName = true;
	}
	if (cmdOptionExists(argv, argv + argc, "-a"))
	{
		if (verbose)
			std::cout << "-a: auto-newline (each message will add a new line even if it does not end with a newline-character)\n";
		settings.autonewline = true;
	}
	if (cmdOptionExists(argv, argv + argc, "-f"))
	{
		if (verbose)
			std::cout << "-f: auto flush (write to disk more often)\n";
		settings.flush = true;
	}
	if (cmdOptionExists(argv, argv + argc, "-c"))
	{
		if (verbose)
			std::cout << "-c: enable console output\n";
		settings.console = true;
	}

	if (cmdOptionExists(argv, argv + argc, "-d"))
	{
		settings.filename = getCmdOption(argv, argv + argc, "-d");
		if (verbose)
			std::cout << "-d: write to: " << settings.filename << "\n";
	}

	if (cmdOptionExists(argv, argv + argc, "-u"))
	{
		if (verbose)
			std::cout << "Send UDP test message...\n";
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
		std::cout << "Listening for OutputDebugString messages..." << std::endl;
		LogMessages(settings);
		std::cout << "Process ended normally.\n";
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
