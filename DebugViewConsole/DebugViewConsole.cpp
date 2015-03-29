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
	if (argc >1)
	{
		// send a UDP test-message (used only for debugging)
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
		std::cout << "DebugViewConsole v" << VERSION_STR << std::endl;
		std::cout << "Listinging for Outputdebugstring messages..." << std::endl;

		fusion::debugviewpp::ShowMessages();
	}
	return 0;
}
catch (std::exception& e)
{
	std::cerr << "Unexpected error occurred in " << e.what() << std::endl;
	std::string message(e.what());
	if (message.find("CreateDBWinBufferMapping") != std::string::npos)
	{
		std::cerr << "Another DebugView++ (or simular application) might be running. " << std::endl;
	}
	return 1;
}
