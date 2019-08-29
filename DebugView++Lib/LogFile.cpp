// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <vector>
#include "CobaltFusion/Str.h"
#include "Win32/Utilities.h"
#include "DebugView++Lib/LogFile.h"

namespace fusion {
namespace debugviewpp {

Message::Message(double time, FILETIME systemTime, DWORD pid, const std::string& processName, const std::string& msg, COLORREF color) :
    time(time),
    systemTime(systemTime),
    processId(pid),
    processName(processName),
    text(msg),
    color(color)
{
}

bool LogFile::Empty() const
{
    return m_messages.empty();
}

void LogFile::Clear()
{
    m_messages.clear();
    m_messages.shrink_to_fit();
    m_storage.Clear();
    m_storage.shrink_to_fit();
    m_processInfo.Clear();
}

void LogFile::Add(const Message& msg)
{
    auto props = m_processInfo.GetProcessProperties(msg.processId, WStr(msg.processName).str());
    m_messages.emplace_back(InternalMessage(msg.time, msg.systemTime, props.uid));
    m_storage.Add(msg.text);
}

int LogFile::BeginIndex() const
{
    return 0;
}

int LogFile::EndIndex() const
{
    return static_cast<int>(m_messages.size());
}

int LogFile::Count() const
{
    return static_cast<int>(m_messages.size());
}

Message LogFile::operator[](int i) const
{
    auto& msg = m_messages[i];
    auto props = m_processInfo.GetProcessProperties(msg.uid);
    return Message(msg.time, msg.systemTime, props.pid, Str(props.name).str(), m_storage[i], props.color);
}

int LogFile::GetHistorySize() const
{
    return m_historySize;
}

void LogFile::SetHistorySize(int size)
{
    m_historySize = size;
}

void LogFile::Append(const LogFile& logfile, int beginIndex, int endIndex)
{
    for (int i = beginIndex; i <= endIndex; ++i)
        Add(logfile[i]);
}

} // namespace debugviewpp
} // namespace fusion
