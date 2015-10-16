// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <cassert>
#include "CobaltFusion/stringbuilder.h"
#include "DebugView++Lib/LineBuffer.h"
#include "DebugView++Lib/SocketReader.h"

namespace fusion {
namespace debugviewpp {

SocketReader::SocketReader(Timer& timer, ILineBuffer& lineBuffer, int port) :
	LogSource(timer, SourceType::UDP_Socket, lineBuffer),
	m_wsa(2, 2),
	m_socket( WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED) ),
	m_event( Win32::CreateEvent(nullptr, false, true, nullptr) ),
	m_busy(false)
{
	SetDescription(wstringbuilder() << L"Listening at UDP port " << port);

	if (m_socket.get() == INVALID_SOCKET)
		Win32::WSAThrowLastError("WSASocket");

	m_overlapped.hEvent = m_event.get();
	m_wsaBuf[0].len = m_buffer.size();
	m_wsaBuf[0].buf = m_buffer.data();

	sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons(port);
	if (bind(m_socket.get(), reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) == SOCKET_ERROR)
		Win32::WSAThrowLastError("bind");
}

HANDLE SocketReader::GetHandle() const
{
	return m_event.get();
}

void SocketReader::Notify()
{
	if (m_busy)
	{
		int len = CompleteReceive();
		Add(0, GetProcessText(), std::string(m_buffer.data(), len));
	}
	Receive();
}

int SocketReader::BeginReceive()
{
	assert(!m_busy);
	DWORD count;
	DWORD flags = 0;
	m_fromLen = sizeof(m_from);
	if (WSARecvFrom(m_socket.get(), m_wsaBuf, 1, &count, &flags, reinterpret_cast<sockaddr*>(&m_from), &m_fromLen, &m_overlapped, nullptr) != SOCKET_ERROR)
		return count;

	auto err = WSAGetLastError();
	if (err != WSA_IO_PENDING)
		Win32::ThrowWin32Error(err, "WSARecvFrom");
	m_busy = true;
	return -1;
}

int SocketReader::CompleteReceive()
{
	assert(m_busy);

	DWORD count;
	DWORD flags = 0;
	int rc = WSAGetOverlappedResult(m_socket.get(), &m_overlapped, &count, FALSE, &flags);
	if (rc == 0)
		Win32::WSAThrowLastError("WSAGetOverlappedResult");

	m_busy = false;
	return count;
}

void SocketReader::Receive()
{
	for (;;)
	{
		int len = BeginReceive();
		if (len < 0)
			break;
		Add(0, GetProcessText(), std::string(m_buffer.data(), len));
	}
}

std::string SocketReader::GetProcessText() const
{
	return stringbuilder() << "[UDP " << inet_ntoa(m_from.sin_addr) << ":" << ntohs(m_from.sin_port) << "]";
}

} // namespace debugviewpp 
} // namespace fusion
