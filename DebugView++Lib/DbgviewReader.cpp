// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Win32/Win32Lib.h"
#include "CobaltFusion/stringbuilder.h"
#include "DebugView++Lib/PassiveLogSource.h"
#include "DebugView++Lib/DbgviewReader.h"
#include "DebugView++Lib/LineBuffer.h"


namespace fusion {
namespace debugviewpp {

const std::string SysinternalsDebugViewAgentPort = "2020";

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

namespace Magic
{
	const int ColumnnOneMark = 1;
	const int ColumnnTwoMark = 2;
	const int Base = 0x83050000;
	const int CaptureKernelEnable = Base + 0x00;              // 0
	const int CaptureKernelDisable = Base + 0x04;             // 1
	const int VerboseKernelMessagesEnable = Base + 0x08;      // 2	// Meaning of these 'VerboseKernel' values was never confirmed
	const int VerboseKernelMessagesDisable = Base + 0x0C;     // 3	// 
	const int PassThroughDisable = Base + 0x10;               // 4
	const int PassThroughEnable = Base + 0x14;                // 5
	const int CaptureWin32Enable = Base + 0x18;               // 6
	const int CaptureWin32Disable = Base + 0x1c;              // 7
	const int Unknown3 = Base + 0x20;                         // 8
	const int RequestUnknown = Base + 0x24;                   // 9	// answer: 0x7fffffff
	const int RequestQueryPerformanceFrequency = Base + 0x28; // A
	const int Unknown4 = Base + 0x2C; 
	const int Unknown5 = Base + 0x30; 
	const int ForceCarriageReturnsEnable = Base + 0x34;
	const int ForceCarriageReturnsDisable = Base + 0x38;
};

template <typename T>
std::string ToHex(const T& s)
{
	std::ostringstream result;
	result << "[" << s.size() << "] ";

	for (size_t i = 0; i < s.size(); ++i)
		result << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << ((int)s[i] & 0xff) << " ";
	return result.str();
}

template <typename T>
std::string ToChar(const T& s)
{
	std::ostringstream result;
	result << "[" << s.size() << "] ";

	for (size_t i = 0; i < s.size(); ++i)
	{
		if (s[i] > 32)
		{
			result << std::setfill(' ') << std::setw(2) << (char)s[i] << " ";
		}
		else
		{
			result << " . ";
		}
	}
	return result.str();
}

void DbgviewReader::SetAutoNewLine(bool value)
{
	LogSource::SetAutoNewLine(value);
	//	 todo: send ForceCarriageReturnsEnable/ForceCarriageReturnsDisable
}

void DbgviewReader::Loop()
{
	m_iostream.connect(m_hostname, SysinternalsDebugViewAgentPort);
	const std::string processName("[tcp]");

	Write<DWORD>(m_iostream, Magic::RequestQueryPerformanceFrequency);
	auto qpFrequency = Read<DWORD>(m_iostream);
	long long t0 = 0;
	bool first = true;

	if (!m_iostream || qpFrequency == 0)
	{
		LogSource::Add(stringbuilder() << "Unable to connect to " << GetDescription() << ", " << m_iostream.error().message());
		Signal();
		return;
	}

	Write<DWORD>(m_iostream, Magic::CaptureKernelEnable);
	Write<DWORD>(m_iostream, Magic::VerboseKernelMessagesEnable);
	Write<DWORD>(m_iostream, Magic::CaptureWin32Enable);
	Write<DWORD>(m_iostream, Magic::PassThroughEnable);
	Write<DWORD>(m_iostream, Magic::ForceCarriageReturnsEnable);

	double timerUnit = 1. / qpFrequency;
	AddMessage(stringbuilder() << "Connected to " << GetDescription());
	Signal();

	while (!AtEnd())
	{
		m_iostream.clear();
		auto messageLength = Read<DWORD>(m_iostream);
		
		if (m_iostream.eof())
		{
			AddMessage(stringbuilder() << "Connection to " << GetDescription() << " closed.");
			LogSource::Abort();
			Signal();
			break;
		}

		if (!m_iostream || messageLength >= 0x7fffffff)
		{
			AddMessage(0, processName, "<error parsing messageLength>");
			Signal();
			break;
		}

		if (messageLength == 0)	// keep alive
			continue;

		// dont read from the tcp::iostream directly,
		// instead use read() to receive the complete message.
		// this allows us to use ss.tellg() to determine the amount of trash bytes.
		std::vector<char> buffer(messageLength);
		m_iostream.read(buffer.data(), messageLength);
		std::stringstream ss(std::ios_base::in | std::ios_base::out | std::ios::binary);
		ss.write(buffer.data(), buffer.size());

		AddMessage(stringbuilder() << ToChar(buffer));
		AddMessage(stringbuilder() << ToHex(buffer));
		Signal();

		DWORD pid = 0;
		std::string msg;
		for (;;)
		{
			unsigned int lineNr = Read<DWORD>(ss);
			if (!ss)
				break;

			auto filetime = Read<FILETIME>(ss);
			auto qpcTime = Read<long long>(ss);
			if (first)
			{
				t0 = qpcTime;
				first = false;
			}
			auto time = (qpcTime - t0) * timerUnit;

			if (buffer[20] == Magic::ColumnnOneMark)
			{
				unsigned char c1, c2;
				if (!((ss >> c1 >> pid >> c2) && c1 == Magic::ColumnnOneMark && c2 == Magic::ColumnnTwoMark))
				{
					AddMessage(0, processName, "<error parsing pid>");
					break;
				}
				auto t = Read(ss, 1);	// discard one leading space
				AddMessage(stringbuilder() << ToChar(t));
				AddMessage(stringbuilder() << ToHex(t));
				Signal();
			}

			// todo: the protocol does not embed \n into the string, so getline works.
			// however, we are missing the information how newlines _are_ send... so for now, we always add a newline
			// tested: dbgview can remote-receive newline and not-newline terminated lines, the difference can be observed by turning 'Option->Force Carriage Returns' off.
			// 
			std::getline(ss, msg, '\0'); 
			msg.push_back('\n');
			AddMessage(time, filetime, pid, processName, msg);

			// strangely, messages are always send in multiples of 4 bytes.
			// this means depending on the message length there are 1, 2 or 3 trailing bytes of undefined data.
			auto remainder = static_cast<int>(ss.tellg() % 4);
			if (remainder > 0)
				Read(ss, 4 - remainder);
		}
		Signal();
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
