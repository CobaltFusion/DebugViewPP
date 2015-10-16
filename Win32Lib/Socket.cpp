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
	WSADATA wsaData = { 0 };
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

} // namespace Win32
} // namespace fusion
