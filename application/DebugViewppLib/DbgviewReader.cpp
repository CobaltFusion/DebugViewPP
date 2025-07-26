// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "Win32/Win32Lib.h"
#include "CobaltFusion/stringbuilder.h"
#include "DebugViewppLib/PolledLogSource.h"
#include "DebugViewppLib/DbgviewReader.h"
#include "DebugViewppLib/LineBuffer.h"

#include <iomanip>
#include <string>
#include <thread>

namespace fusion {
namespace debugviewpp {

const std::string SysinternalsDebugViewAgentPort = "2020";

DbgviewReader::DbgviewReader(Timer& timer, ILineBuffer& linebuffer, const std::string& hostname) :
    PolledLogSource(timer, SourceType::Pipe, linebuffer, 40),
    m_hostname(hostname),
    m_thread([this] { Loop(); })
{
    SetDescription(wstringbuilder() << "Dbgview Agent at " << m_hostname);
}

std::vector<unsigned char> Read(std::stringstream& is, size_t amount)
{
    if (amount < 1)
    {
        return std::vector<unsigned char>();
    }
    std::vector<unsigned char> buffer(amount);
    is.read(reinterpret_cast<char*>(buffer.data()), amount);
    return buffer;
}

namespace Magic {
const DWORD ColumnnOneMark = 1;
const DWORD ColumnnTwoMark = 2;
const DWORD Base = 0x83050000;
const DWORD CaptureKernelEnable = Base + 0x00;              // 0
const DWORD CaptureKernelDisable = Base + 0x04;             // 1
const DWORD VerboseKernelMessagesEnable = Base + 0x08;      // 2    // Meaning of these 'VerboseKernel' values was never confirmed
const DWORD VerboseKernelMessagesDisable = Base + 0x0C;     // 3    //
const DWORD PassThroughDisable = Base + 0x10;               // 4
const DWORD PassThroughEnable = Base + 0x14;                // 5
const DWORD CaptureWin32Enable = Base + 0x18;               // 6
const DWORD CaptureWin32Disable = Base + 0x1c;              // 7
const DWORD Unknown3 = Base + 0x20;                         // 8
const DWORD RequestUnknown = Base + 0x24;                   // 9    // answer: 0x7fffffff
const DWORD RequestQueryPerformanceFrequency = Base + 0x28; // A
const DWORD Unknown4 = Base + 0x2C;
const DWORD Unknown5 = Base + 0x30;
const DWORD ForceCarriageReturnsEnable = Base + 0x34;
const DWORD ForceCarriageReturnsDisable = Base + 0x38;
} // namespace Magic

template <typename T>
std::string ToHex(const T& s)
{
    std::ostringstream result;
    result << "[" << s.size() << "] ";

    for (size_t i = 0; i < s.size(); ++i)
        result << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << (static_cast<int>(s[i]) & 0xff) << " ";
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
            result << std::setfill(' ') << std::setw(2) << static_cast<char>(s[i]) << " ";
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
    //     todo: send ForceCarriageReturnsEnable/ForceCarriageReturnsDisable to dbgview-agent
}

void DbgviewReader::Loop()
{
    SetDescription(wstringbuilder() << "Dbgview Agent at " << m_hostname);
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

    if (GetAutoNewLine())
    {
        Write<DWORD>(m_iostream, Magic::ForceCarriageReturnsEnable);
    }
    else
    {
        Write<DWORD>(m_iostream, Magic::ForceCarriageReturnsDisable);
    }

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

        if (messageLength == 0)
        { // keep alive
            continue;
        }

        // dont read from the tcp::iostream directly,
        // instead use read() to receive the complete message.
        // this allows us to use ss.tellg() to determine the amount of trash bytes.
        std::vector<char> buffer(messageLength);
        m_iostream.read(buffer.data(), messageLength);
        std::stringstream ss(std::ios_base::in | std::ios_base::out | std::ios::binary);
        ss.write(buffer.data(), buffer.size());

        DWORD pid = 0;
        std::string msg;
        for (;;)
        {
            Read<DWORD>(ss); // lineNr
            if (!ss)
            {
                break;
            }

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
                unsigned char c1 = 0;
                unsigned char c2 = 0;
                if (!((ss >> c1 >> pid >> c2) && c1 == Magic::ColumnnOneMark && c2 == Magic::ColumnnTwoMark))
                {
                    AddMessage(0, processName, "<error parsing pid>");
                    break;
                }
                Read(ss, 1); // discard one leading space
            }

            std::getline(ss, msg, '\0');
            msg.push_back('\n'); // newlines are never send as part of the message
            AddMessage(time, filetime, pid, processName, msg);

            // strangely, messages are always send in multiples of 4 bytes.
            // this means depending on the message length there are 1, 2 or 3 trailing bytes of undefined data.
            auto remainder = static_cast<int>(ss.tellg() % 4);
            if (remainder > 0)
            {
                Read(ss, 4 - remainder);
            }
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
