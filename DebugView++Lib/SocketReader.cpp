// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DebugView++Lib/PassiveLogSource.h"
#include "DebugView++Lib/SocketReader.h"
#include "DebugView++Lib/LineBuffer.h"

#include <boost/asio.hpp> 
#include <boost/array.hpp>

namespace fusion {
namespace debugviewpp {

SocketReader::SocketReader(Timer& timer, ILineBuffer& linebuffer) :
	PassiveLogSource(timer, SourceType::Pipe, linebuffer, 40)
{
	SetDescription(L"socket");

	m_thread = boost::thread(&SocketReader::Loop, this);
}

SocketReader::~SocketReader()
{
}

void SocketReader::Loop()
{
	using namespace boost::asio;
	io_service io_service; 
	ip::tcp::resolver resolver(io_service);
	ip::tcp::socket socket(io_service); 

	ip::tcp::resolver::query localAgent("127.0.0.1", "2020"); 
	auto endpoint_iterator = resolver.resolve(localAgent);
	ip::tcp::resolver::iterator end;
	boost::system::error_code error = boost::asio::error::host_not_found;

    while (error && endpoint_iterator != end)
    {
      socket.close();
      socket.connect(*endpoint_iterator++, error);
    }
    if (error)
      throw boost::system::system_error(error);

	boost::array<unsigned char, 20> startBuf = { 
		0x24, 0x00, 0x05, 0x83,
		0x04, 0x00, 0x05, 0x83,
		0x08, 0x00, 0x05, 0x83,
		0x28, 0x00, 0x05, 0x83, //  (reponse: 90 ae 23 00 00 00 00 00)
		0x18, 0x00, 0x05, 0x83  //  (reponse: 00 00 00 00)	
	} ;
	socket.write_some(boost::asio::buffer(startBuf), error);

	for(;;)
	{
		boost::array<unsigned char, 5000> buf;
		size_t len = socket.read_some(boost::asio::buffer(buf), error);
		if (error == boost::asio::error::eof)
			break; // Connection closed cleanly by peer.
		else if (error)
			throw boost::system::system_error(error); // Some other error.

		std::string testmsg;

		std::cout << "recv " << len << " bytes: ";
		for(size_t i=0; i<len; ++i)
		{
			std::cout << "0x" << std::hex << std::uppercase << (unsigned int)buf[i] << " ";
		}
		std::cout << std::endl;

		size_t i=0;
		unsigned int command = buf[i++];
		command += buf[i++] << 8;
		command += buf[i++] << 16;
		command += buf[i++] << 24;
		switch (command)
		{
			case 0:
				// keepalife
				std::cout << "*keepalive*" << std::endl;
				Add(0, "debugview", "*keepalive*\n", this);
				break;
			default:
				{
				// msg

					std::string msg = stringbuilder() << "[" << len << "] ";
					for (; i<len; ++i)
					{
						char c = (char) buf[i];
						std::cout << i << ": [0x" << std::hex << std::uppercase << (unsigned int)buf[i] << "] " << c << std::endl;
						if (c >=32 && c <128)
							msg.push_back(c);
					}
					msg.push_back('\n');
					Add(0, "debugview", msg.c_str(), this);
					
					//std::string msg((char*)&buf[i+16]);
					//std::cout << "Msg: " << msg << std::endl;
				}
				break;
		}	
			
		if (AtEnd())
			break;
		Signal();
	}
}

void SocketReader::Abort()
{
	LogSource::Abort();
	m_thread.join();
}

void SocketReader::Poll()
{
	// http://stackoverflow.com/questions/15126277/boost-tcp-socket-with-shared-ptr-c
	// http://www.boost.org/doc/libs/1_47_0/doc/html/boost_asio/examples.html#boost_asio.examples.http_server
	// http://www.boost.org/doc/libs/1_47_0/doc/html/boost_asio/example/iostreams/http_client.cpp
	// http://www.boost.org/doc/libs/1_46_0/doc/html/boost_asio/example/timeouts/blocking_tcp_client.cpp

}

} // namespace debugviewpp 
} // namespace fusion
