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

#define HEX(x) std::setfill('0') << std::setw(2) << std::hex << std::uppercase << unsigned int(x) << std::setw(0)

std::vector<unsigned char> Read(boost::asio::ip::tcp::iostream& is, size_t amount)
{
	if (amount < 1)
		return std::vector<unsigned char>();
	std::vector<unsigned char> buffer(amount);
	is.read((char*)buffer.data(), amount);
	return buffer;
}

std::vector<unsigned char> ReadRemaining(boost::asio::ip::tcp::iostream& is)
{
	std::vector<unsigned char> buffer(10000);
	auto len = is.readsome((char*)buffer.data(), buffer.size());
	buffer.resize(unsigned int(len));
	return buffer;
}

void SocketReader::Loop()
{
	using namespace boost::asio;
	//io_service io_service; 
	//ip::tcp::resolver resolver(io_service);
	//ip::tcp::socket socket(io_service); 

	//ip::tcp::resolver::query localAgent("127.0.0.1", "2020"); 
	//auto endpoint_iterator = resolver.resolve(localAgent);
	//ip::tcp::resolver::iterator end;
	//boost::system::error_code error = boost::asio::error::host_not_found;

 //   while (error && endpoint_iterator != end)
 //   {
 //     socket.close();
 //     socket.connect(*endpoint_iterator++, error);
 //   }
 //   if (error)
 //     throw boost::system::system_error(error);

	ip::tcp::iostream is("127.0.0.1", "2020");		// much shorter, but not direct socket access.
	//boost::iostreams:: stream<asio_stream_device> is(socket);

    if (!is)
    {
      std::cout << "Unable to connect: " << is.error().message() << std::endl;
      return;
    }
	
	boost::array<unsigned char, 20> startBuf = { 
		0x24, 0x00, 0x05, 0x83,
		0x04, 0x00, 0x05, 0x83,
		0x08, 0x00, 0x05, 0x83,
		0x28, 0x00, 0x05, 0x83, //  (reponse: 90 ae 23 00 00 00 00 00)
		0x18, 0x00, 0x05, 0x83  //  (reponse: 00 00 00 00)	
	};

	is.write((char*)startBuf.data(), startBuf.size());
	if (!is)
	{
		std::cout << "Unable to connect: " << is.error().message() << std::endl;
		Add(0, "dbgview.exe", "<error sending init command>\n", this);
	}

	for(;;)
	{
		is.clear();
		auto command = Read<DWORD>(is);
		if (!is)
		{
			Add(0, "dbgview.exe", "<error parsing command>\n", this);
			break;
		}

		bool multilineMessage = false;
        switch (command)
		{
			case 0:
				// keepalive
				//std::cout << "*keepalive*" << std::endl;
				//Add(0, "debugview", "*keepalive*\n", this);
				break;
			case 0x7fffffff:
				// init reply 1
				Add(0, "dbgview.exe", "*reply 1*\n", this);
				break;
			case 0x0023ae93:
				// init reply 2
				Add(0, "dbgview.exe", "*reply 2*\n", this);
				break;
			//case 0x80:
				multilineMessage = true;
				std::cout << "multilineMessage<true>" << std::endl;
			//case 0x28:
				{
					std::cout << "command: " << HEX(command) << std::endl;
					DWORD pid = 0;
					std::string msg, flags;
					for(;;)
					{
						std::cout << "==== next === " << std::endl;
						msg.clear();
						flags.clear();
						
						unsigned int lineNr = Read<DWORD>(is);

						if (is.eof())
						{
							std::cout << " is has eof!" << std::endl;
						}

						if (is.fail())
						{
							std::cout << " is has failed!" << std::endl;
						}
						if (!is)
						{
							std::cout << " is false!" << std::endl;
						}

						if (!is)
						{
							Add(0, "dbgview.exe", "<end of messagecollection>\n", this);
							break;
						}

						auto ufo = Read(is, 16);	// yet to decode timestamp

						unsigned char c1, c2;
						if (!((is >> c1 >> pid >> c2) && c1 == 0x1 && c2 == 0x2))
						{
							Add(0, "dbgview.exe", "<error parsing pid>\n", this);
							break;
						}
						std::getline(is, msg, '\0'); 
						std::cout << "lineNr: " << lineNr << " msg: " << msg.c_str() << std::endl;


						msg.push_back('\n');
						Add(pid, "dbgview.exe", msg.c_str(), this);
						
						if (multilineMessage)
							std::getline(is, flags, '\0'); 
						else
						{
							auto ufo = Read(is, 1);	 //flags?
							std::cout << "multilineMessage == false, stopping..." << std::endl;

							break;
						}
					}
					std::cout << "==== end of messages === " << std::endl;
				}
				break;
			default:
				{
					std::cout << "unknown command: " << HEX(command) << std::endl;
					auto buf = ReadRemaining(is);
					auto it = buf.cbegin();
					while(it != buf.cend())
					{
						unsigned char c = *it++;
						std::cout << HEX(c) << " ";
						if (c == 0)
							std::cout << std::endl;
					}
					std::cout << std::endl;
				}
				break;
		}	
		Signal();
		if (AtEnd())
			break;
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
