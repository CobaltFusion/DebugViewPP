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

namespace Win32 {

class WinsockInitialization : boost::noncopyable
{
public:
	explicit WinsockInitialization(int major = 2, int minor = 2)
	{
		WSADATA wsaData = { 0 };
		int rc = WSAStartup(MAKEWORD(major, minor), &wsaData);
		if (rc != 0)
			ThrowWin32Error(rc, "WSAStartup");
	}

	~WinsockInitialization()
	{
		WSACleanup();
	}
};

void WSAThrowLastError(const std::string& what)
{
	Win32::ThrowWin32Error(WSAGetLastError(), what);
}

struct SocketDeleter
{
	typedef SOCKET pointer;

	void operator()(pointer p) const
	{
		if (p != INVALID_SOCKET)
			closesocket(p);
	}
};

typedef std::unique_ptr<void, SocketDeleter> Socket;

} // namespace Win32

namespace debugviewpp {

class SocketReader2 : public LogSource
{
public:
	explicit SocketReader2(Timer& timer, SourceType::type sourceType, ILineBuffer& linebuffer, int port) :
		LogSource(timer, sourceType, linebuffer),
		m_wsa(2, 2),
		m_socket( WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED) ),
		m_event( Win32::CreateEvent(nullptr, false, false, nullptr) )
	{
		if (m_socket.get() == INVALID_SOCKET)
			Win32::WSAThrowLastError("WSASocket");

		sockaddr_in sa;
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = htonl(INADDR_ANY);
		sa.sin_port = htons(port);
		if (bind(m_socket.get(), reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) == SOCKET_ERROR)
			Win32::WSAThrowLastError("bind");

	}

	virtual HANDLE GetHandle() const
	{
		return m_event.get();
	}

	virtual void Notify()
	{
		Add(m_buffer.data());
	}

	bool BeginReceive()
	{
		WSABUF buffers[] = { m_buffer.size(), m_buffer.data() };

		DWORD count;
		DWORD flags;
		sockaddr_in from;
		int fromlen;
		WSAOVERLAPPED overlapped;
		overlapped.hEvent = m_event.get();
		if (WSARecvFrom(m_socket.get(), buffers, 1, &count, &flags, reinterpret_cast<sockaddr*>(&from), &fromlen, &overlapped, nullptr) == SOCKET_ERROR)
		{
			auto err = WSAGetLastError();
			if (err != WSA_IO_PENDING)
				Win32::ThrowWin32Error(err, "WSARecvFrom");
			return false;
		}
		return true;
	}

	void CompleteReceive()
	{
		DWORD count;
		DWORD flags;
		WSAOVERLAPPED overlapped;
		overlapped.hEvent = m_event.get();
		int rc = WSAGetOverlappedResult(m_socket.get(), &overlapped, &count, FALSE, &flags);
		if (rc == 0)
			Win32::WSAThrowLastError("WSAGetOverlappedResult");
	}

private:
	Win32::WinsockInitialization m_wsa;
	Win32::Socket m_socket;
	Win32::Handle m_event;
	boost::array<char, 2000> m_buffer;
};

void OverlappedUdpReaderSample(int port)
{
	Win32::WinsockInitialization wsa(2, 2);

	Win32::Socket s( WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED) );
	if (s.get() == INVALID_SOCKET)
		Win32::WSAThrowLastError("WSASocket");

	sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons(port);
	if (bind(s.get(), reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) == SOCKET_ERROR)
		Win32::WSAThrowLastError("bind");

	char buffer[2000];
	WSABUF buffers[] = { sizeof(buffer), buffer };

	DWORD count;
	DWORD flags;
	sockaddr_in from;
	int fromlen;
	auto hEvent = Win32::CreateEvent(nullptr, false, false, nullptr);
	WSAOVERLAPPED overlapped;
	overlapped.hEvent = hEvent.get();
	if (WSARecvFrom(s.get(), buffers, 1, &count, &flags, reinterpret_cast<sockaddr*>(&from), &fromlen, &overlapped, nullptr) == SOCKET_ERROR)
	{
		auto err = WSAGetLastError();
		if (err != WSA_IO_PENDING)
			Win32::ThrowWin32Error(err, "WSARecvFrom");

		Win32::WaitForSingleObject(hEvent);
		int rc = WSAGetOverlappedResult(s.get(), &overlapped, &count, FALSE, &flags);
		if (rc == 0)
			Win32::WSAThrowLastError("WSAGetOverlappedResult");
	}

}

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
