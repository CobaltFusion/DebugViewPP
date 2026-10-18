// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <atomic>
#include <cstdint>
#include <istream>
#include <streambuf>
#include <string>
#include <vector>
#include "Win32/Socket.h"

namespace fusion {
namespace Win32 {

// A blocking TCP client connection that is used as a std::iostream (a small replacement for boost::asio::ip::tcp::iostream).
//
//   TcpStream stream;
//   if (stream.Connect("localhost", 2020)) { stream.write(...); stream.read(...); }
//
// - Connect resolves a host name or an IP address (IPv4 and IPv6) and tries every address. On failure the stream is in the
//   fail state and ErrorMessage() tells why.
// - Writes are sent immediately (unitbuf), reading blocks until data is available. A read at the end of the connection sets eofbit.
// - Close can be called from another thread: a thread that is blocked in a read (that read sees the end of the
//   stream) or in Connect notices it within 100 ms. After Close the stream cannot be reused. The socket itself is released by the destructor.
class TcpStream : public std::iostream
{
public:
    TcpStream();
    ~TcpStream() override;

    TcpStream(const TcpStream&) = delete;
    TcpStream& operator=(const TcpStream&) = delete;

    bool Connect(const std::string& host, uint16_t port);
    void Attach(SOCKET socket); // takes ownership of an accepted connection, see TcpListener
    void Close();
    std::string ErrorMessage() const;

private:
    class StreamBuf : public std::streambuf
    {
    public:
        StreamBuf();

        bool Connect(const std::string& host, uint16_t port);
        void Attach(SOCKET socket);
        void Close();
        std::string ErrorMessage() const;

    protected:
        int_type underflow() override;
        int_type overflow(int_type ch) override;
        int sync() override;

    private:
        bool Send(const char* data, size_t size);
        bool Flush();
        void SetError(int error);

        WinsockInitialization m_wsa;
        Socket m_socket;
        std::atomic<SOCKET> m_handle{INVALID_SOCKET}; // the same as m_socket, readable from another thread
        std::atomic<bool> m_closed{false};
        std::atomic<int> m_error{0};
        std::vector<char> m_inBuffer;
        std::vector<char> m_outBuffer;
    };

    StreamBuf m_buf;
};

// Accepts TCP connections on a port of the loopback interface (or of all interfaces).
class TcpListener
{
public:
    TcpListener(uint16_t port, bool loopbackOnly = true);

    // Waits for a connection, returns false when Close was called (also from another thread).
    bool Accept(TcpStream& stream);
    void Close();

private:
    WinsockInitialization m_wsa;
    Socket m_socket;
    std::atomic<bool> m_closed{false};
};

// Sends a UDP broadcast message to 255.255.255.255 on the specified port.
void SendBroadcast(uint16_t port, const std::string& message);

} // namespace Win32
} // namespace fusion
