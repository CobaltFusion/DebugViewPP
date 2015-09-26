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
	is.read(reinterpret_cast<char*>(buffer.data()), amount);
	return buffer;
}

struct Magic
{
	enum type
	{
		colomnOneMark = 1,
		colomnTwoMark = 2,
		base = 0x83050000,
		captureKernelEnable = base + 0x00,				// 0
		captureKernelDisable = base + 0x04,				// 1
		unknown1 = base + 0x08,							// 2 
		unknown2 = base + 0x0C,							// 3 
		passThroughEnable = base + 0x10,				// 4
		passThroughDisable = base + 0x14,				// 5
		captureWin32Enable = base + 0x18,				// 6
		captureWin32Disable = base + 0x1c,				// 7
		unknown3 = base + 0x20,							// 8 
		requestUnknown = base + 0x24,					// 9 
		requestQueryPerformanceFrequency = base + 0x28	// A
	};
};

void DbgviewReader::Loop()
{
	m_iostream.connect(m_hostname, g_sysinternalsDebugViewAgentPort);
	const char* processName = "[tcp]";

	// unknown command (dbgview sends it after connect, it gets a 4 byte answer 0x7fffffff)
	//Write<DWORD>(m_iostream, Magic::base + 0x24);	
	//Read<DWORD>(m_iostream);					// 0x7fffffff

	// unknown command (dbgview sends it after connect, but without it all seems fine too)
	// maybe 0x08 turns something on, and 0x0C turns it off? (following the logic of the enums)
	//Write<DWORD>(m_iostream, Magic::base + 0x08);	

	Write<DWORD>(m_iostream, Magic::requestQueryPerformanceFrequency);
	auto qpFrequency = Read<DWORD>(m_iostream);		// 0x0023ae93

	if (!m_iostream || qpFrequency == 0)
	{
	  Add(stringbuilder() << "Unable to connect to " << GetDescription() << ", " << m_iostream.error().message());
	  Signal();
	  return;
	}

	Write<DWORD>(m_iostream, Magic::captureKernelEnable);
	Write<DWORD>(m_iostream, Magic::captureWin32Disable);
	Write<DWORD>(m_iostream, Magic::passThroughDisable);

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
		for (;;)
		{
			msg.clear();
			flags.clear();
						
			unsigned int lineNr = Read<DWORD>(ss);
			if (!ss)
				break;
			
			auto filetime = Read<FILETIME>(ss);
			auto qpcTime = Read<long long>(ss);
			auto time = timer.Get(qpcTime);

			if (buffer[20] == Magic::colomnOneMark)
			{
				unsigned char c1, c2;
				if (!((ss >> c1 >> pid >> c2) && c1 == Magic::colomnOneMark && c2 == Magic::colomnTwoMark))
				{
					Add(0, processName, "<error parsing pid>\n");
					break;
				}
				Read(ss, 1);	// discard one leading space
			}

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
