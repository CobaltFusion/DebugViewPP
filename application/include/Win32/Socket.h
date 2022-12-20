// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memory>
#include "winsock2.h"
#include "Win32/Win32Lib.h"

namespace fusion {
namespace Win32 {

class WinsockInitialization : noncopyable
{
public:
    explicit WinsockInitialization(int major = 2, int minor = 2);
    ~WinsockInitialization();
};

void WSAThrowLastError(const std::string& what);

struct SocketDeleter
{
    using pointer = SOCKET;

    void operator()(pointer p) const;
};

using Socket = std::unique_ptr<void, SocketDeleter>;

Socket WSASocket(int af, int type, int protocol);
Socket WSASocket(int af, int type, int protocol, const WSAPROTOCOL_INFO* pProtocolInfo, GROUP g, DWORD flags);

void bind(Socket& socket, const sockaddr_in& sa);

bool WSARecvFrom(Socket& s, const WSABUF buffers[], DWORD bufferCount, DWORD* pNumberOfBytesRecvd, DWORD* pFlags, sockaddr_in& from, int& fromLen, WSAOVERLAPPED* pOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE pCompletionRoutine);

DWORD WSAGetOverlappedResult(Socket& s, WSAOVERLAPPED& overlapped, bool wait, DWORD& flags);

} // namespace Win32
} // namespace fusion
