// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <vector>
#include "CobaltFusion/Str.h"
#include "Win32/Utilities.h"
#include "DebugViewppLib/LogFile.h"

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
    m_firstIndex = 0;
    m_storage.Clear();
    m_storage.shrink_to_fit();
    m_processInfo.Clear();
}

void LogFile::Add(const Message& msg)
{
    auto props = m_processInfo.GetProcessProperties(msg.processId, WStr(msg.processName).str());
    m_messages.emplace_back(InternalMessage(msg.time, msg.systemTime, props.uid));
    m_storage.Add(msg.text);
    RemoveOldMessages();
}

// removes the oldest messages when there are more than the history size, the storage removes complete blocks of messages
// (when that does not remove anything yet, this has no effect, so it can be called for every message)
void LogFile::RemoveOldMessages()
{
    if (m_historySize <= 0 || static_cast<int>(m_messages.size()) <= m_historySize)
    {
        return;
    }

    const auto removed = m_storage.DropFront(m_messages.size() - static_cast<size_t>(m_historySize));
    if (removed > 0)
    {
        m_messages.erase(m_messages.begin(), m_messages.begin() + static_cast<std::ptrdiff_t>(removed));
        m_firstIndex += static_cast<int>(removed);
    }
}

int LogFile::BeginIndex() const
{
    return m_firstIndex;
}

int LogFile::EndIndex() const
{
    return m_firstIndex + static_cast<int>(m_messages.size());
}

int LogFile::Count() const
{
    return static_cast<int>(m_messages.size());
}

Message LogFile::operator[](int i) const
{
    auto& msg = m_messages[i - m_firstIndex];
    auto props = m_processInfo.GetProcessProperties(msg.uid);
    return Message(msg.time, msg.systemTime, props.pid, Str(props.name).str(), m_storage[i], props.color);
}

int LogFile::GetHistorySize() const
{
    return m_historySize;
}

void LogFile::SetHistorySize(int size)
{
    m_historySize = std::max(size, 0);
    RemoveOldMessages();
}

void LogFile::Append(const LogFile& logfile, int beginIndex, int endIndex)
{
    for (int i = beginIndex; i <= endIndex; ++i)
    {
        Add(logfile[i]);
    }
}

} // namespace debugviewpp
} // namespace fusion
