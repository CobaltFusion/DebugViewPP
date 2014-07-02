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

typedef std::vector<unsigned char> Buffer;


DWORD GetDWORD(Buffer::const_iterator& it)
{
	DWORD value = *(it++);
	value += *(it++) << 8;
	value += *(it++) << 16;
	value += *(it++) << 24;
	return value;
}

std::string GetString(Buffer::const_iterator& it)
{
	std::string value;
	while((*it) >= 10)
		value.push_back(*(it++));
	return value;
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
		std::vector<unsigned char> buf(5000);

		size_t len = socket.read_some(boost::asio::buffer(buf), error);
		buf.resize(len);
		if (error == boost::asio::error::eof)
			break; // Connection closed cleanly by peer.
		else if (error)
			throw boost::system::system_error(error); // Some other error.

		std::string testmsg;

		//std::cout << "recv " << len << " bytes: ";
		//for(size_t i=0; i<len; ++i)
		//{
		//	std::cout << "0x" << std::hex << std::uppercase << (unsigned int)buf[i] << " ";
		//}
		//std::cout << std::endl;

		unsigned int command = GetDWORD(buf.cbegin());
        switch (command)
		{
			case 0:
				// keepalife
				//std::cout << "*keepalive*" << std::endl;
				//Add(0, "debugview", "*keepalive*\n", this);
				break;
			case 0x7fffffff:
				// init reply 1
				Add(0, "debugview", "*reply 1*\n", this);
				break;
			case 0x0023ae93:
				// init reply 2
				Add(0, "debugview", "*reply 2*\n", this);
				break;
			default:
				{
    				// msg
		            unsigned int lineNr = GetDWORD(buf.cbegin() + 4); //	 we dont need the linenumbers, but they could serve as an integrety check during debugging

                    if (len < 17)       // unknown msg
                        break;

					auto it = buf.cbegin() + 25;
                    std::string pid = "pid: " + GetString(it);
					it++;
					auto msgIter = it;
					std::string msg = stringbuilder() << lineNr << " (" << len << ") " + GetString(msgIter);

					while(it != buf.cend())
						std::cout << std::hex << std::uppercase << std::setfill('.') << std::setw(2) << (unsigned int) *it++ << std::left << " ";

                    unsigned char flags = buf[len-1];
                    bool newline = (flags & 1) != 0;

					msg += stringbuilder() << " " << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << unsigned int(flags) << std::left << " ";
                    if (newline)
                        msg += " <1>";
                    else
                        msg += " <0>";

                    std::cout << std::endl;
					msg.push_back('\n');
                    Add(0, pid.c_str(), msg.c_str(), this);
					
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
