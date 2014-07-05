// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DebugView++Lib/PassiveLogSource.h"
#include "DebugView++Lib/SocketReader.h"
#include "DebugView++Lib/LineBuffer.h"
#include "Win32Lib/Win32Lib.h"

#include <boost/asio.hpp> 

namespace fusion {
namespace debugviewpp {

SocketReader::SocketReader(Timer& timer, ILineBuffer& linebuffer) :
	PassiveLogSource(timer, SourceType::Pipe, linebuffer, 40)
{
	SetDescription(L"socket");

	m_thread = boost::thread(&SocketReader::Loop, this);
}

SocketReader::~SocketReader()
{
}

#define HEX(x) std::setfill('0') << std::setw(2) << std::hex << std::uppercase << unsigned int(x) << std::setw(0)

std::vector<unsigned char> Read(boost::asio::ip::tcp::iostream& is, size_t amount)
{
	if (amount < 1)
		return std::vector<unsigned char>();
	std::vector<unsigned char> buffer(amount);
	is.read((char*)buffer.data(), amount);
	return buffer;
}

void DebugVector(std::vector<unsigned char> buffer)
{
	for (size_t i=0; i< buffer.size(); ++i)
	{
		std::cout << HEX(buffer[i]) << " ";
		if ((i != 0) && ((i & 15) == 0))
			std::cout << std::endl;
	}
	std::cout << std::endl;
}

std::vector<unsigned char> Read(std::stringstream& is, size_t amount, bool debug = false)
{
	if (amount < 1)
		return std::vector<unsigned char>();
	std::vector<unsigned char> buffer(amount);
	is.read((char*)buffer.data(), amount);

	if (debug)	// todo remove this code when we're sure we've got the dbgview protocol down
		DebugVector(buffer);
	return buffer;
}

std::wstring FormatDateTime2(const SYSTEMTIME& systemTime)
{
	int size = GetTimeFormat(LOCALE_USER_DEFAULT, 0, &systemTime, nullptr, nullptr, 0);
	size += GetDateFormat(LOCALE_USER_DEFAULT, 0, &systemTime, nullptr, nullptr, 0);
	std::vector<wchar_t> buf(size);

	int offset = GetDateFormat(LOCALE_USER_DEFAULT, 0, &systemTime, nullptr, buf.data(), size);
	buf[offset - 1] = ' ';
	GetTimeFormat(LOCALE_USER_DEFAULT, 0, &systemTime, nullptr, buf.data() + offset, size);
	return std::wstring(buf.data(), size - 1);

}
std::wstring FormatDateTime2(const FILETIME& fileTime)
{
	return FormatDateTime2(FileTimeToSystemTime(FileTimeToLocalFileTime(fileTime)));
}

void SocketReader::Loop()
{
	using namespace boost::asio;
	ip::tcp::iostream is("127.0.0.1", "2020");		// much shorter, but not direct socket access.

    if (!is)
    {
      std::cout << "Unable to connect: " << is.error().message() << std::endl;
      return;
    }
	
	boost::array<unsigned char, 20> startBuf = { 
		0x24, 0x00, 0x05, 0x83,
		0x04, 0x00, 0x05, 0x83,
		0x08, 0x00, 0x05, 0x83,
		0x28, 0x00, 0x05, 0x83,
		0x18, 0x00, 0x05, 0x83 
	};

	is.write((char*)startBuf.data(), startBuf.size());
	if (!is)
	{
		Add(0, "dbgview.exe", "<error sending init command>\n", this);
	}

	Read<DWORD>(is);					// 0x7fffffff		// Init reply
	auto qpFrequency = Read<DWORD>(is);	// 0x0023ae93		// QueryPerformanceFrequency

	Timer timer(qpFrequency);

	for(;;)
	{
		is.clear();
		auto messageLength = Read<DWORD>(is);
		
		if (!is || messageLength >= 0x7fffffff)
		{
			Add(0, "dbgview.exe", "<error parsing messageLength>\n", this);
			break;
		}

		if (messageLength == 0)	// keep alive
			continue;

		// dont read from the tcp::iostream directly,
		// instead use read() to receive the complete message.
		// this allows us to use ss.tellg() to determine the amount of trash bytes.
		std::vector<char> buffer(messageLength);
		is.read(reinterpret_cast<char*>(buffer.data()), messageLength);
		std::stringstream ss(std::ios_base::in | std::ios_base::out | std::ios::binary);
		ss.write(buffer.data(), buffer.size());

		DWORD pid = 0;
		std::string msg, flags;
		for(;;)
		{
			msg.clear();
			flags.clear();
						
			unsigned int lineNr = Read<DWORD>(ss);
			if (!ss)
				break;
			
			auto filetime = Read<FILETIME>(ss);
			auto qpcTime = Read<long long>(ss);
			auto time = timer.Get(qpcTime);

			unsigned char c1, c2;
			if (!((ss >> c1 >> pid >> c2) && c1 == 0x1 && c2 == 0x2))
			{
				Add(0, "dbgview.exe", "<error parsing pid>\n", this);
				break;
			}
			Read(ss, 1);	// discard one leading space
			std::getline(ss, msg, '\0'); 

			msg.push_back('\n');
			Add(time, filetime, pid, "dbgview.exe", msg.c_str(), this);

			// strangely, messages are always send in multiples of 4 bytes.
			// this means depending on the message length there are 1, 2 or 3 trailing bytes of undefined data.
			auto pos = (size_t) ss.tellg();
			auto remainder = pos % 4;
			if (remainder > 0)
			{
				auto trashBytes = 4 - remainder;
				Read(ss, trashBytes);	// discard trailing trash 
			}
		}
		Signal();
		if (AtEnd())
			break;
	}
}

void SocketReader::Abort()
{
	LogSource::Abort();
	m_thread.join();
}

void SocketReader::Poll()
{
	// http://stackoverflow.com/questions/15126277/boost-tcp-socket-with-shared-ptr-c
	// http://www.boost.org/doc/libs/1_47_0/doc/html/boost_asio/examples.html#boost_asio.examples.http_server
	// http://www.boost.org/doc/libs/1_47_0/doc/html/boost_asio/example/iostreams/http_client.cpp
	// http://www.boost.org/doc/libs/1_46_0/doc/html/boost_asio/example/timeouts/blocking_tcp_client.cpp

}

} // namespace debugviewpp 
} // namespace fusion
