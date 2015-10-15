// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <boost/asio.hpp> 
#include "Win32/Win32Lib.h"
#include "CobaltFusion/stringbuilder.h"
#include "DebugView++Lib/PassiveLogSource.h"
#include "DebugView++Lib/SocketReader.h"
#include "DebugView++Lib/LineBuffer.h"


namespace fusion {
namespace debugviewpp {

SocketReader::SocketReader(Timer& timer, ILineBuffer& lineBuffer, int port) :
	PassiveLogSource(timer, SourceType::Pipe, lineBuffer, 40),
	m_port(port),
	m_socket(m_ioservice, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port))
{
	SetDescription(wstringbuilder() << "Listening at UDP port " << m_port);
	m_thread = boost::thread(&SocketReader::Loop, this);
}

SocketReader::~SocketReader()
{
	m_ioservice.stop();
}

void SocketReader::ReceiveUDPMessage(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	std::stringstream ss(std::ios_base::in | std::ios_base::out | std::ios::binary);
	ss.write(m_recvBuffer.data(), bytes_transferred);

	std::string msg;
	while (std::getline(ss, msg, '\0'))
	{
		if (!msg.empty())
		{
			msg.push_back('\n');
			AddMessage(0, stringbuilder() << "[UDP " << m_remote_endpoint.address() << ":" << m_port << "]", msg);
			Signal();
		}
	}
	StartReceive();
}

void SocketReader::StartReceive()
{
	m_socket.async_receive_from(boost::asio::buffer(m_recvBuffer), m_remote_endpoint,
		boost::bind(&SocketReader::ReceiveUDPMessage, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
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
