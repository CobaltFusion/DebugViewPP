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
#include "DBWinReader.h"
#include "ProcessInfo.h"
#include "LogSources.h"
#include "../DebugView++/version.h"

namespace fusion {
namespace debugviewpp {

void ShowMessages()
{
	std::vector<std::unique_ptr<LogSource>> sources;
	
	sources.push_back(make_unique<DBWinReader>(false));
	if (HasGlobalDBWinReaderRights())
		sources.push_back(make_unique<DBWinReader>(true));

	auto pred = [](const Line& a, const Line& b) { return a.time < b.time; };
	for (;;)
	{
		Lines lines;
		for (auto it = sources.begin(); it != sources.end(); )
		{
			Lines pipeLines((*it)->GetLines());
			Lines lines2;
			lines2.reserve(lines.size() + pipeLines.size());
			std::merge(lines.begin(), lines.end(), pipeLines.begin(), pipeLines.end(), std::back_inserter(lines2), pred);
			lines.swap(lines2);

			if ((*it)->AtEnd())
				it = sources.erase(it);
			else
				++it;
		}

		for (auto i=lines.begin(); i != lines.end(); ++i)
		{
			std::cout << i->pid << "\t" << i->processName.c_str() << "\t" << i->message.c_str() << "\n";
		}
		Sleep(100);
	}
}

void OnMessage(double time, FILETIME systemTime, DWORD processId, HANDLE processHandle, const char* message)
{
	static boost::mutex mutex;
	boost::mutex::scoped_lock lock(mutex);
	Handle hanel(processHandle);
	std::string processName = Str(ProcessInfo::GetProcessName(processHandle)).str();
	std::cout << processId << "\t" << processName << "\t" << message << "\n";
}

void Method2()
{
	boost::signals2::connection connection1;
	boost::signals2::connection connection2;

	LogSources sources(false);
	//boost::thread t([&]() { Sleep(5000); sources.Abort(); });  // test stopping after 5 seconds

	auto dbwinlistener = make_unique<DBWinReader>(false);
	connection1 = dbwinlistener->Connect(&OnMessage);
	sources.Add(std::move(dbwinlistener));			

	std::cout << "Logging DBWin32 Messages to stdout...\n";

	if (HasGlobalDBWinReaderRights())
	{
		auto globalDBbwinlistener = make_unique<DBWinReader>(true);
		connection2 = globalDBbwinlistener->Connect(&OnMessage);
		sources.Add(std::move(globalDBbwinlistener));
		std::cout << "Logging Global DBWin32 Messages to stdout...\n";
	}
	sources.Listen();
}

} // namespace debugviewpp
} // namespace fusion

int main(int argc, char* argv[])
try
{
	std::cout << "DebugViewConsole v" << VERSION_STR << "\n"; 
	fusion::debugviewpp::ShowMessages();
	return 0;
}
catch (std::exception& e)
{
	std::cerr << "DebugViewCmd error:" << e.what() << std::endl;
	return 1;
}
