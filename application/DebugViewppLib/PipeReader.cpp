// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "Win32/Win32Lib.h"
#include "CobaltFusion/stringbuilder.h"
#include "DebugViewppLib/PipeReader.h"
#include "DebugViewppLib/LineBuffer.h"
#include <array>

namespace fusion {
namespace debugviewpp {

PipeReader::PipeReader(Timer& timer, ILineBuffer& linebuffer, HANDLE hPipe, DWORD pid, const std::string& processName, long pollFrequency) :
    PolledLogSource(timer, SourceType::Pipe, linebuffer, pollFrequency),
    m_hPipe(hPipe),
    m_pid(pid),
    m_process(processName)
{
    SetDescription(wstringbuilder() << L"Piped from " << processName);
    StartThread();
}

PipeReader::~PipeReader() = default;

bool PipeReader::AtEnd() const
{
    return LogSource::AtEnd() || PeekNamedPipe(m_hPipe, nullptr, 0, nullptr, nullptr, nullptr) == FALSE;
}

void PipeReader::Poll()
{
    Poll(*this);
}

void PipeReader::Poll(PolledLogSource& logsource)
{
    std::array<char, 4096> buf;

    // copy m_buffer into 'buf', set 'start' to the position one past the last element copied
    char* start = std::copy(m_buffer.data(), m_buffer.data() + m_buffer.size(), buf.data());

    DWORD avail = 0;
    while ((PeekNamedPipe(m_hPipe, nullptr, 0, nullptr, &avail, nullptr) != 0) && avail > 0)
    {
        auto size = static_cast<DWORD>(buf.data() + sizeof(buf) - start);
        DWORD read = 0;
        ReadFile(m_hPipe, start, size, &read, nullptr);

        char* begin = buf.data();
        char* end = start + read;
        char* p = start;
        while (p != end)
        {
            if (*p == '\0' || *p == '\n' || p - begin > 4000)
            {
                logsource.AddMessage(m_pid, m_process, std::string(begin, p));
                begin = p + 1;
            }
            ++p;
        }
        start = std::copy(begin, end, buf.data());
    }
    // keep remainer of line for next poll()
    m_buffer = std::string(buf.data(), start);
}


} // namespace debugviewpp
} // namespace fusion
