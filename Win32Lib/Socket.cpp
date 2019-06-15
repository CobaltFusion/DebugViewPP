// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Win32/Win32Lib.h"
#include "Win32/Socket.h"

namespace fusion {
namespace Win32 {

WinsockInitialization::WinsockInitialization(int major, int minor)
{
	WSADATA wsaData = {};
	int rc = WSAStartup(MAKEWORD(major, minor), &wsaData);
	if (rc != 0)
		ThrowWin32Error(rc, "WSAStartup");
}

WinsockInitialization::~WinsockInitialization()
{
	WSACleanup();
}

void WSAThrowLastError(const std::string& what)
{
	Win32::ThrowWin32Error(WSAGetLastError(), what);
}

void SocketDeleter::operator()(pointer p) const
{
	if (p != INVALID_SOCKET)
		closesocket(p);
}

Socket WSASocket(int af, int type, int protocol)
{
	return WSASocket(af, type, protocol, nullptr, 0, 0);
}

Socket WSASocket(int af, int type, int protocol, const WSAPROTOCOL_INFO* pProtocolInfo, GROUP g, DWORD flags)
{
	SOCKET s = ::WSASocket(af, type, protocol, const_cast<WSAPROTOCOL_INFO*>(pProtocolInfo), g, flags);
	if (s == INVALID_SOCKET)
		WSAThrowLastError("WSASocket");
	return Socket(s);
}

void bind(Socket& socket, const sockaddr_in& sa)
{
	if (::bind(socket.get(), reinterpret_cast<const sockaddr*>(&sa), sizeof(sa)) == SOCKET_ERROR)
		WSAThrowLastError("bind");
}

bool WSARecvFrom(Socket& s, const WSABUF buffers[], DWORD bufferCount, DWORD* pNumberOfBytesRecvd, DWORD* pFlags, sockaddr_in& from, int& fromLen, WSAOVERLAPPED* pOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE pCompletionRoutine)
{
	if (::WSARecvFrom(s.get(), const_cast<WSABUF*>(buffers), bufferCount, pNumberOfBytesRecvd, pFlags, reinterpret_cast<sockaddr*>(&from), &fromLen, pOverlapped, pCompletionRoutine) != SOCKET_ERROR)
		return true;

	auto err = WSAGetLastError();
	if (err != WSA_IO_PENDING)
		ThrowWin32Error(err, "WSARecvFrom");
	return false;
}

DWORD WSAGetOverlappedResult(Socket& s, WSAOVERLAPPED& overlapped, bool wait, DWORD& flags)
{
	DWORD count;
	if (!::WSAGetOverlappedResult(s.get(), &overlapped, &count, wait, &flags))
		Win32::WSAThrowLastError("WSAGetOverlappedResult");
	return count;
}

} // namespace Win32
} // namespace fusion
