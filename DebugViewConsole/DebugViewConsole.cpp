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
#include "CobaltFusion/ExecutorClient.h"
#include "CobaltFusion/Executor.h"
#include "DebugView++Lib/DBWinBuffer.h"
#include "DebugView++Lib/DBWinReader.h"
#include "DebugView++Lib/FileIO.h"
#include "DebugView++Lib/ProcessInfo.h"
#include "DebugView++Lib/LogSources.h"
#include "DebugView++Lib/Conversions.h"
#include "DebugView++Lib/LineBuffer.h"
#include "../DebugView++/version.h"

#include "DebugView++Lib/Filter.h"
#include "DebugView++Lib/LogFile.h"

#define DOCOPT_HEADER_ONLY
#include "docopt.h"

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
	bool linenumber;
	bool console;
	bool verbose;
	std::string filename;
    std::vector<std::string> include;
    std::vector<std::string> exclude;
    std::vector<std::string> includeprocesses;
    std::vector<std::string> excludeprocesses;
    std::string quitmessage;
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
static Win32::Handle g_quitMessageHandle;

void Quit()
{
	g_quit = true;
}

bool ContainsText(const std::string& line, const std::string& message)
{
	if (message.empty()) return false;
	return line.find(message) != std::string::npos;
}

bool IsEventSet(Win32::Handle& handle)
{
	return Win32::WaitForSingleObject(handle, 0);
}

void AddMessageFilter(LogFilter& filter, FilterType::type filterType, const std::string pattern)
{
    auto bgColor = COLORREF();
    auto fgColor = COLORREF();
    filter.messageFilters.push_back(Filter(pattern, MatchType::Simple, filterType, bgColor, fgColor));
}

void AddProcessFilter(LogFilter& filter, FilterType::type filterType, const std::string pattern)
{
    auto bgColor = COLORREF();
    auto fgColor = COLORREF();
    filter.processFilters.push_back(Filter(pattern, MatchType::Simple, filterType, bgColor, fgColor));
}

bool IsIncluded(LogFilter& filter, const Line& line)
{
    MatchColors matchcolors; //  not used on the command-line
    return IsIncluded(filter.processFilters, line.processName, matchcolors) && IsIncluded(filter.messageFilters, line.message, matchcolors);
}

void LogMessages(Settings settings)
{
	using namespace std::chrono_literals;
	LogFilter filter;
	ActiveExecutorClient executor;
	LogSources logsources(executor);
	executor.Call([&] {
		logsources.AddDBWinReader(false);
		if (IsWindowsVistaOrGreater() && HasGlobalDBWinReaderRights())
			logsources.AddDBWinReader(true);
		logsources.SetAutoNewLine(settings.autonewline);
	});

	for (const auto& value : settings.include)
	{
		AddMessageFilter(filter, FilterType::Include, value);
	}

	for (const auto& value : settings.exclude)
	{
		AddMessageFilter(filter, FilterType::Exclude, value);
	}

	for (const auto& value : settings.includeprocesses)
	{
		AddProcessFilter(filter, FilterType::Include, value);
	}

	for (const auto& value : settings.excludeprocesses)
	{
		AddProcessFilter(filter, FilterType::Exclude, value);
	}

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
	while (!g_quit && (!IsEventSet(g_quitMessageHandle)))
	{
		Lines lines;
		executor.Call([&] {
			lines = logsources.GetLines();
		});
		int linenumber = 0;
		for (const auto& line : lines)
		{
			if (ContainsText(line.message, settings.quitmessage))
			{
				Quit();
				break;
			}

			if (!debugviewpp::IsIncluded(filter, line))
				continue;

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

static const char USAGE[] =
R"(DebugviewConsole )" VERSION_STR
R"(
    Usage:
        DebugviewConsole [-acflsqtpnv] [-d <file>] [-i <pattern>]... [-e <pattern>]... [-m <message>] [--include-process <pattern>]... [--exclude-process <pattern>]...
        DebugviewConsole (-h | --help)
        DebugviewConsole [-x]
        DebugviewConsole [-u]

    Options:
        -h, --help                          show this screen
        -i <pattern>, --include <pattern>   include filter, may be specified multiple times
        -e <pattern>, --exclude <pattern>   exclude filter, may be specified multiple times
        --include-process <pattern>         include filter for process names
        --exclude-process <pattern>         exclude filter for process names
        -a              auto-newline, most people want this
        -c              enable console output
        -d <file>       write to .dblog file
        -v              verbose

    Console options:    (no effect on .dblog file)
        -l              prefix line number
        -s              prefix messages with system time
        -q              prefix message with high-precision (<1us) offset from QueryPerformanceCounter
        -t              tab-separated output
        -p              add PID (process ID)
        -n              add process name

    Advanced options:
        -f              aggressively flush buffers, if unsure, do not use
        -x              stop all running debugviewconsole instances
        -u              send a UDP test-message, used only for debugging
        -m <message>, --quit-message <message>  if this message is received the application exits
)";

fusion::debugviewpp::Settings CreateSettings(const std::map<std::string, docopt::value>& args)
{
	auto settings = fusion::debugviewpp::Settings();
	auto filenameEntry = args.at("-d");
	settings.filename = (filenameEntry) ? filenameEntry.asString() : "";
	settings.autonewline = args.at("-a").asBool();
	settings.flush = args.at("-f").asBool();
	settings.console = args.at("-c").asBool();
	settings.verbose = args.at("-v").asBool();
	settings.linenumber = args.at("-l").asBool();
	settings.timestamp = args.at("-s").asBool();
	settings.performanceCounter = args.at("-t").asBool();
	settings.tabs = args.at("-q").asBool();
	settings.pid = args.at("-p").asBool();
	settings.processName = args.at("-n").asBool();
	settings.include = args.at("--include").asStringList();
	settings.exclude = args.at("--exclude").asStringList();
	settings.includeprocesses = args.at("--include-process").asStringList();
	settings.excludeprocesses = args.at("--exclude-process").asStringList();
	auto quitmessageEntry = args.at("--quit-message");
	settings.quitmessage = (quitmessageEntry) ? quitmessageEntry.asString() : "";
	return settings;
}

int main(int argc, char* argv[])
try
{
	using namespace fusion::debugviewpp;

	// install CTRL-C handler
	SetConsoleCtrlHandler(ConsoleHandler, TRUE);

	const auto args = docopt::docopt(USAGE, {argv + 1, argv + argc}, true, "DebugviewConsole " VERSION_STR);
	auto settings = CreateSettings(args);

	if (settings.verbose)
	{
		for (auto const& arg : args)
		{
			if (arg.second.isStringList())
			{
				std::cout << arg.first << "  is a list: " << arg.second << std::endl;
			}
			else
				std::cout << arg.first << " " << arg.second << std::endl;
		}
	}

	std::cout << "DebugViewConsole " << VERSION_STR << std::endl;

	g_quitMessageHandle = fusion::Win32::CreateEvent(nullptr, true, false, L"DebugViewConsoleQuitEvent");
	if (args.at("-x").asBool())
	{
		if (settings.verbose)
			std::cout << "-x: sending terminate signal to all DebugViewConsole instances\n";
		SetEvent(g_quitMessageHandle);
		return 0;
	}

	if (args.at("-u").asBool())
	{
		std::string msg = argv[2];
		msg += "\n";
		std::cout << "Broadcast to 255.255.255.255 on UDP port 2020, message: " << msg;
		using namespace boost::asio::ip;
		boost::asio::io_service io_service;
		udp::resolver resolver(io_service);
		udp::resolver::query query(udp::v4(), "255.255.255.255", "2020");
		udp::endpoint receiver_endpoint = *resolver.resolve(query);
		udp::socket socket(io_service);
		socket.open(udp::v4());

		// enable broadcast
		boost::asio::socket_base::broadcast option(true);
		socket.set_option(option);

		socket.send_to(boost::asio::buffer(msg), receiver_endpoint);
		std::cout << "Done." << std::endl;
		return 0;
	}

	if (settings.filename.empty() && settings.console == false)
	{
		std::cout << "Neither output to logfile or console was specified, noting to do...\n";
		return 1;
	}

	std::cout << "Listening for OutputDebugString messages..." << std::endl;
	LogMessages(settings);
	std::cout << "Process ended normally.\n";
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
