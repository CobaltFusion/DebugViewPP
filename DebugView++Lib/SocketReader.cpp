// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DebugView++Lib/PassiveLogSource.h"
#include "DebugView++Lib/SocketReader.h"
#include "DebugView++Lib/LineBuffer.h"
#include "Win32Lib/Win32Lib.h"

#include <boost/asio.hpp> 

namespace fusion {
namespace debugviewpp {

using namespace boost::asio::ip;

SocketReader::SocketReader(Timer& timer, ILineBuffer& linebuffer, const std::string& hostname, const std::string& port) :
	PassiveLogSource(timer, SourceType::Pipe, linebuffer, 40),
	m_hostname(hostname),
	m_port(port),
	m_socket(m_ioservice, udp::endpoint(udp::v4(), 2999))
{
	SetDescription(wstringbuilder() << "Listing socket at UDP port '" << m_port << "'");
	m_thread = boost::thread(&SocketReader::Loop, this);
}

SocketReader::~SocketReader()
{
	m_ioservice.stop();
}

void SocketReader::ReceiveUDPMessage(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	std::stringstream ss(std::ios_base::in | std::ios_base::out | std::ios::binary);
	ss.write(m_RecvBuffer.data(), bytes_transferred);

	std::string addr = m_remote_endpoint.address().to_string();
	std::string msg = addr + ": ";
	for(;;)
	{
		std::getline(ss, msg, '\0'); 
		if (!msg.empty())
		{
			msg = addr + ": " + msg + "\n";
			std::string port = stringbuilder() << "[UDP " << m_port << "]";
			Add(0, port.c_str(), msg.c_str());
			Signal();
		}
		if (!ss)
			break;
	}
	StartReceive();
}

void SocketReader::StartReceive()
{
	m_socket.async_receive_from(
		boost::asio::buffer(m_RecvBuffer), m_remote_endpoint,
		boost::bind(&SocketReader::ReceiveUDPMessage, this,
		  boost::asio::placeholders::error,
		  boost::asio::placeholders::bytes_transferred)
	);    
}

void SocketReader::Loop()
{
	StartReceive();
	Add(stringbuilder() << GetDescription() << "\n");
	Signal();
	m_ioservice.run();
}

void SocketReader::Abort()
{
	m_ioservice.stop();
	LogSource::Abort();
	//m_thread.join();
}

} // namespace debugviewpp 
} // namespace fusion
