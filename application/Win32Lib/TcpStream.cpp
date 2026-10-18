// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "winsock2.h"
#include "ws2tcpip.h"
#include "windows.h"

#include <chrono>
#include <system_error>
#include "Win32/TcpStream.h"

#pragma comment(lib, "ws2_32.lib")

namespace fusion {
namespace Win32 {

namespace {

constexpr size_t BufferSize = 4096;

// blocking calls wait in slices, so that Close can be noticed
constexpr int PollIntervalMs = 100;
constexpr auto ConnectTimeout = std::chrono::seconds(20);

bool WaitForSocket(SOCKET s, bool forWrite, const std::atomic<bool>& closed, std::chrono::steady_clock::time_point deadline, int& error)
{
    for (;;)
    {
        if (closed)
        {
            error = WSAECONNABORTED;
            return false;
        }

        fd_set set;
        FD_ZERO(&set);
        FD_SET(s, &set);
        fd_set errorSet = set;
        timeval timeout = {0, PollIntervalMs * 1000};
        int rc = forWrite ? select(0, nullptr, &set, &errorSet, &timeout) : select(0, &set, nullptr, &errorSet, &timeout);
        if (rc == SOCKET_ERROR)
        {
            error = WSAGetLastError();
            return false;
        }
        if (rc > 0)
        {
            return true;
        }
        if (std::chrono::steady_clock::now() >= deadline)
        {
            error = WSAETIMEDOUT;
            return false;
        }
    }
}

SOCKET ConnectTo(const addrinfo& ai, const std::atomic<bool>& closed, int& error)
{
    SOCKET s = ::socket(ai.ai_family, ai.ai_socktype, ai.ai_protocol);
    if (s == INVALID_SOCKET)
    {
        error = WSAGetLastError();
        return INVALID_SOCKET;
    }

    u_long nonBlocking = 1;
    ioctlsocket(s, FIONBIO, &nonBlocking);
    bool connected = ::connect(s, ai.ai_addr, static_cast<int>(ai.ai_addrlen)) == 0;
    if (!connected)
    {
        error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK)
        {
            connected = WaitForSocket(s, true, closed, std::chrono::steady_clock::now() + ConnectTimeout, error);
            if (connected)
            {
                // the result of the connect attempt
                int soError = 0;
                int len = sizeof(soError);
                getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&soError), &len);
                if (soError != 0)
                {
                    error = soError;
                    connected = false;
                }
            }
        }
    }

    if (!connected)
    {
        closesocket(s);
        return INVALID_SOCKET;
    }

    nonBlocking = 0;
    ioctlsocket(s, FIONBIO, &nonBlocking);
    return s;
}

} // namespace

TcpStream::StreamBuf::StreamBuf() :
    m_inBuffer(BufferSize),
    m_outBuffer(BufferSize)
{
    setg(m_inBuffer.data(), m_inBuffer.data(), m_inBuffer.data());
    setp(m_outBuffer.data(), m_outBuffer.data() + m_outBuffer.size());
}

void TcpStream::StreamBuf::SetError(int error)
{
    int expected = 0;
    m_error.compare_exchange_strong(expected, error);
}

bool TcpStream::StreamBuf::Connect(const std::string& host, uint16_t port)
{
    if (m_handle != INVALID_SOCKET)
    {
        SetError(WSAEISCONN);
        return false;
    }

    addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* pResult = nullptr;
    int rc = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &pResult);
    if (rc != 0)
    {
        SetError(rc);
        return false;
    }

    int error = WSAHOST_NOT_FOUND;
    SOCKET s = INVALID_SOCKET;
    for (const addrinfo* pAddress = pResult; pAddress != nullptr && s == INVALID_SOCKET && !m_closed; pAddress = pAddress->ai_next)
    {
        s = ConnectTo(*pAddress, m_closed, error);
    }
    freeaddrinfo(pResult);

    if (s == INVALID_SOCKET)
    {
        SetError(error);
        return false;
    }

    Attach(s);
    return true;
}

void TcpStream::StreamBuf::Attach(SOCKET socket)
{
    m_socket.reset(socket);
    m_handle = socket;
    if (m_closed)
    {
        shutdown(m_handle, SD_BOTH);
    }
}

// The socket is only shut down here (the peer sees the end of the stream) and closed by the destructor, so the handle can
// not be reused by another socket while a thread is still using it. A thread that waits in underflow or Connect polls m_closed.
void TcpStream::StreamBuf::Close()
{
    m_closed = true;
    const SOCKET s = m_handle;
    if (s != INVALID_SOCKET)
    {
        shutdown(s, SD_BOTH);
    }
}

std::string TcpStream::StreamBuf::ErrorMessage() const
{
    const int error = m_error;
    return error == 0 ? std::string() : std::system_category().message(error);
}

std::streambuf::int_type TcpStream::StreamBuf::underflow()
{
    if (gptr() < egptr())
    {
        return traits_type::to_int_type(*gptr());
    }

    if (m_handle == INVALID_SOCKET || !Flush())
    {
        return traits_type::eof();
    }

    // shutdown() does not wake up a blocking recv, so wait in slices and look at the closed flag in between
    int error = 0;
    if (!WaitForSocket(m_handle, false, m_closed, std::chrono::steady_clock::time_point::max(), error))
    {
        if (!m_closed)
        {
            SetError(error);
        }
        return traits_type::eof();
    }

    int count = recv(m_handle, m_inBuffer.data(), static_cast<int>(m_inBuffer.size()), 0);
    if (count <= 0)
    {
        if (count < 0 && !m_closed)
        {
            SetError(WSAGetLastError());
        }
        return traits_type::eof();
    }

    setg(m_inBuffer.data(), m_inBuffer.data(), m_inBuffer.data() + count);
    return traits_type::to_int_type(*gptr());
}

std::streambuf::int_type TcpStream::StreamBuf::overflow(int_type ch)
{
    if (!Flush())
    {
        return traits_type::eof();
    }
    if (!traits_type::eq_int_type(ch, traits_type::eof()))
    {
        *pptr() = traits_type::to_char_type(ch);
        pbump(1);
    }
    return traits_type::not_eof(ch);
}

int TcpStream::StreamBuf::sync()
{
    return Flush() ? 0 : -1;
}

bool TcpStream::StreamBuf::Send(const char* data, size_t size)
{
    while (size > 0)
    {
        int count = send(m_handle, data, static_cast<int>(size), 0);
        if (count == SOCKET_ERROR)
        {
            if (!m_closed)
            {
                SetError(WSAGetLastError());
            }
            return false;
        }
        data += count;
        size -= count;
    }
    return true;
}

bool TcpStream::StreamBuf::Flush()
{
    const auto size = static_cast<size_t>(pptr() - pbase());
    if (size == 0)
    {
        return true;
    }

    setp(m_outBuffer.data(), m_outBuffer.data() + m_outBuffer.size());
    return m_handle != INVALID_SOCKET && Send(m_outBuffer.data(), size);
}

TcpStream::TcpStream() :
    std::iostream(&m_buf)
{
    setf(std::ios_base::unitbuf);
}

TcpStream::~TcpStream() = default;

bool TcpStream::Connect(const std::string& host, uint16_t port)
{
    clear();
    if (!m_buf.Connect(host, port))
    {
        setstate(std::ios_base::failbit);
        return false;
    }
    return true;
}

void TcpStream::Attach(SOCKET socket)
{
    clear();
    m_buf.Attach(socket);
}

void TcpStream::Close()
{
    m_buf.Close();
}

std::string TcpStream::ErrorMessage() const
{
    return m_buf.ErrorMessage();
}

TcpListener::TcpListener(uint16_t port, bool loopbackOnly) :
    m_socket(WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
{
    BOOL reuse = TRUE;
    setsockopt(m_socket.get(), SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in sa = {};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(loopbackOnly ? INADDR_LOOPBACK : INADDR_ANY);
    bind(m_socket, sa);

    if (listen(m_socket.get(), SOMAXCONN) == SOCKET_ERROR)
    {
        WSAThrowLastError("listen");
    }
}

bool TcpListener::Accept(TcpStream& stream)
{
    int error = 0;
    const auto deadline = std::chrono::steady_clock::time_point::max();
    if (!WaitForSocket(m_socket.get(), false, m_closed, deadline, error))
    {
        return false;
    }

    SOCKET s = accept(m_socket.get(), nullptr, nullptr);
    if (s == INVALID_SOCKET)
    {
        return false;
    }
    stream.Attach(s);
    return true;
}

void TcpListener::Close()
{
    m_closed = true;
}

void SendBroadcast(uint16_t port, const std::string& message)
{
    WinsockInitialization wsa;
    Socket socket = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    BOOL broadcast = TRUE;
    if (setsockopt(socket.get(), SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&broadcast), sizeof(broadcast)) == SOCKET_ERROR)
    {
        WSAThrowLastError("setsockopt(SO_BROADCAST)");
    }

    sockaddr_in sa = {};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    if (sendto(socket.get(), message.data(), static_cast<int>(message.size()), 0, reinterpret_cast<const sockaddr*>(&sa), sizeof(sa)) == SOCKET_ERROR)
    {
        WSAThrowLastError("sendto");
    }
}

} // namespace Win32
} // namespace fusion
