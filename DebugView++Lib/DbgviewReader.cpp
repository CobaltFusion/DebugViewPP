// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DebugView++Lib/PassiveLogSource.h"
#include "DebugView++Lib/DbgviewReader.h"
#include "DebugView++Lib/LineBuffer.h"
#include "Win32Lib/Win32Lib.h"

#include <boost/asio.hpp> 

namespace fusion {
namespace debugviewpp {

const std::string g_sysinternalsDebugViewAgentPort = "2020";

DbgviewReader::DbgviewReader(Timer& timer, ILineBuffer& linebuffer, const std::string& hostname) :
	PassiveLogSource(timer, SourceType::Pipe, linebuffer, 40),
	m_hostname(hostname)
{
	SetDescription(wstringbuilder() << "Dbgview Agent at " << m_hostname);
	m_thread = boost::thread(&DbgviewReader::Loop, this);
}

DbgviewReader::~DbgviewReader()
{
}

std::vector<unsigned char> Read(std::stringstream& is, size_t amount)
{
	if (amount < 1)
		return std::vector<unsigned char>();
	std::vector<unsigned char> buffer(amount);
	is.read((char*)buffer.data(), amount);
	return buffer;
}

void DbgviewReader::Loop()
{
	m_iostream.connect(m_hostname, g_sysinternalsDebugViewAgentPort);
	const char* processName = "[tcp]";

	boost::array<unsigned char, 20> startBuf = { 
		0x24, 0x00, 0x05, 0x83,
		0x04, 0x00, 0x05, 0x83,
		0x08, 0x00, 0x05, 0x83,
		0x28, 0x00, 0x05, 0x83,
		0x18, 0x00, 0x05, 0x83 
	};

	m_iostream.write((char*)startBuf.data(), startBuf.size());
	Read<DWORD>(m_iostream);					// 0x7fffffff		// Init reply
	auto qpFrequency = Read<DWORD>(m_iostream);	// 0x0023ae93		// QueryPerformanceFrequency

    if (!m_iostream || qpFrequency == 0)
    {
	  Add(stringbuilder() << "Unable to connect to " << GetDescription() << ", " << m_iostream.error().message());
	  Signal();
      return;
    }

	Timer timer(qpFrequency);
	Add(stringbuilder() << "Connected to " << GetDescription());
	Signal();

	for(;;)
	{
		m_iostream.clear();
		auto messageLength = Read<DWORD>(m_iostream);
		
		if (m_iostream.eof())
		{
			Add(stringbuilder() << "Connected to " << GetDescription() << " closed.");
			LogSource::Abort();
			break;
		}

		if (!m_iostream || messageLength >= 0x7fffffff)
		{
			Add(0, processName, "<error parsing messageLength>\n");
			Signal();
			break;
		}

		if (messageLength == 0)	// keep alive
			continue;

		// dont read from the tcp::iostream directly,
		// instead use read() to receive the complete message.
		// this allows us to use ss.tellg() to determine the amount of trash bytes.
		std::vector<char> buffer(messageLength);
		m_iostream.read(reinterpret_cast<char*>(buffer.data()), messageLength);
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
				Add(0, processName, "<error parsing pid>\n");
				break;
			}
			Read(ss, 1);	// discard one leading space
			std::getline(ss, msg, '\0'); 

			msg.push_back('\n');
			Add(time, filetime, pid, processName, msg.c_str());

			// strangely, messages are always send in multiples of 4 bytes.
			// this means depending on the message length there are 1, 2 or 3 trailing bytes of undefined data.
			auto remainder = ((size_t) ss.tellg()) % 4;
			if (remainder > 0)
				Read(ss, 4 - remainder);
		}
		Signal();
		if (AtEnd())
			break;
	}
}

void DbgviewReader::Abort()
{
	m_iostream.close();
	LogSource::Abort();
	m_thread.join();
}

} // namespace debugviewpp 
} // namespace fusion
