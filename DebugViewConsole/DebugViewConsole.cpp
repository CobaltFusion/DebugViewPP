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

namespace fusion {
namespace debugviewpp {

void ShowMessages()
{
	LogSources sources(true);
	sources.AddDBWinReader(false);
	if (HasGlobalDBWinReaderRights())
		sources.AddDBWinReader(true);

	for (;;)
	{
		auto lines = sources.GetLines();
		for (auto i=lines.begin(); i != lines.end(); ++i)
		{
			std::cout << i->pid << "\t" << i->processName.c_str() << "\t" << i->message.c_str() << "\n";
		}
		Sleep(100);
	}
}

} // namespace debugviewpp
} // namespace fusion

int main(int argc, char* argv[])
try
{
	std::cout << "DebugViewConsole v" << VERSION_STR << std::endl;
	fusion::debugviewpp::ShowMessages();
	return 0;
}
catch (std::exception& e)
{
	std::cerr << "DebugViewConsole error:" << e.what() << std::endl;
	return 1;
}
