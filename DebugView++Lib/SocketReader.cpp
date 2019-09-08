// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <cassert>
#include "CobaltFusion/stringbuilder.h"
#include "DebugView++Lib/SocketReader.h"

namespace fusion {
namespace debugviewpp {

SocketReader::SocketReader(Timer& timer, ILineBuffer& lineBuffer, int port) :
    LogSource(timer, SourceType::Udp, lineBuffer),
    m_wsa(2, 2),
    m_socket(Win32::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED)),
    m_event(Win32::CreateEvent(nullptr, false, true, nullptr)),
    m_fromLen(0),
    m_busy(false)
{
    SetDescription(wstringbuilder() << L"Listening at UDP port " << port);

    m_overlapped.hEvent = m_event.get();
    m_wsaBuf[0].len = static_cast<LONG>(m_buffer.size());
    m_wsaBuf[0].buf = m_buffer.data();

    sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(port);
    Win32::bind(m_socket, sa);
}

HANDLE SocketReader::GetHandle() const
{
    return m_event.get();
}

std::string bufferToString(const char* buffer, int size)
{
    size_t length = strnlen(buffer, size);
    return std::string(buffer, length);
}

void SocketReader::Notify()
{
    if (m_busy)
    {
        int len = CompleteReceive();
        Add(0, GetProcessText(), bufferToString(m_buffer.data(), len));
    }

    for (;;)
    {
        int len = BeginReceive();
        if (len < 0)
        {
            return;
        }
        Add(0, GetProcessText(), bufferToString(m_buffer.data(), len));
    }
}

int SocketReader::BeginReceive()
{
    assert(!m_busy);
    DWORD count;
    DWORD flags = 0;
    m_fromLen = sizeof(m_from);
    if (Win32::WSARecvFrom(m_socket, m_wsaBuf, 1, &count, &flags, m_from, m_fromLen, &m_overlapped, nullptr))
    {
        return count;
    }

    m_busy = true;
    return -1;
}

int SocketReader::CompleteReceive()
{
    assert(m_busy);
    DWORD flags = 0;
    auto count = Win32::WSAGetOverlappedResult(m_socket, m_overlapped, false, flags);
    m_busy = false;
    return count;
}

std::string SocketReader::GetProcessText() const
{
    return stringbuilder() << "[UDP " << inet_ntoa(m_from.sin_addr) << ":" << ntohs(m_from.sin_port) << "]";
}

} // namespace debugviewpp
} // namespace fusion
