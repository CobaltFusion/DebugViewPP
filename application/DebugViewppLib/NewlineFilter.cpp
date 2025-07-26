// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "CobaltFusion/stringbuilder.h"
#include "DebugViewppLib/LogSource.h"
#include "DebugViewppLib/ProcessInfo.h"
#include "DebugViewppLib/NewlineFilter.h"
#include <iostream>

namespace fusion {
namespace debugviewpp {

Lines NewlineFilter::Process(const Line& line)
{
    auto& message = m_lineBuffers[line.pid];
    message.reserve(512);

    Lines lines;
    for (auto c : line.message)
    {
        if (c == '\r')
        {
            continue;
        }

        if (c == '\n')
        {
            Line outputLine(line.time, line.systemTime, line.pid, line.processName, "", line.pLogSource);
            std::swap(outputLine.message, message);
            lines.emplace_back(std::move(outputLine));
        }
        else
        {
            message.push_back(c);
        }
    }

    if (!message.empty())
    {
        if (line.pLogSource->GetAutoNewLine() || message.size() > 8192) // 8k line limit prevents stack overflow in handling code
        {
            Line outputLine(line.time, line.systemTime, line.pid, line.processName, "", line.pLogSource);
            std::swap(outputLine.message, message);
            lines.emplace_back(std::move(outputLine));
        }
    }
    return lines;
}

Lines NewlineFilter::FlushLinesFromTerminatedProcess(DWORD pid, HANDLE /*handle*/) // todo: why is handle unused?
{
    Lines lines;
    if (m_lineBuffers.find(pid) != m_lineBuffers.end())
    {
        if (!m_lineBuffers[pid].empty())
        {
            // timestamp not filled, this will be done by the loopback source
            lines.push_back(Line(0, FILETIME(), pid, "<flush>", m_lineBuffers[pid], nullptr));
        }
        m_lineBuffers.erase(pid);
    }
    return lines;
}

} // namespace debugviewpp
} // namespace fusion
