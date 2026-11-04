// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <array>
#include "Win32/Win32Lib.h"
#include "Win32/Socket.h"
#include "LogSource.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

class SocketReader : public LogSource
{
public:
    SocketReader(Timer& timer, ILineBuffer& lineBuffer, int port);

    [[nodiscard]] HANDLE GetHandle() const override;
    void Notify() override;

private:
    int BeginReceive();
    int CompleteReceive();
    [[nodiscard]] std::string GetProcessText() const;

    Win32::WinsockInitialization m_wsa;
    Win32::Socket m_socket;
    Win32::Handle m_event;
    // room for the largest UDP datagram over IPv4 (65507 bytes): a datagram larger than the buffer fails the receive with WSAEMSGSIZE,
    // which ends the thread that reads all sources
    std::array<char, 65536> m_buffer;
    WSAOVERLAPPED m_overlapped = WSAOVERLAPPED();
    WSABUF m_wsaBuf[1]{};
    sockaddr_in m_from = sockaddr_in();
    int m_fromLen;
    bool m_busy;
};

} // namespace debugviewpp
} // namespace fusion
