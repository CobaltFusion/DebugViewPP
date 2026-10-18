// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <vector>
#include "DebugviewppLib/Colors.h"
#include "DebugviewppLib/ProcessInfo.h"
#include "IndexedStorageLib/IndexedStorage.h"

namespace fusion {
namespace debugviewpp {

struct Message
{
    Message(double time, FILETIME systemTime, DWORD pid, const std::string& processName, const std::string& msg, COLORREF color = Colors::BackGround);

    double time;
    FILETIME systemTime;
    DWORD processId;
    std::string processName;
    std::string text;
    COLORREF color;
};

class LogFile
{
public:
    bool Empty() const;
    void Clear();
    void Add(const Message& msg);
    void Append(const LogFile& logfile, int beginIndex, int endIndex);
    // The index of a message does not change, when old messages are removed because of the history size the first index increases.
    // The messages that are available have an index from BeginIndex() up to (not including) EndIndex()
    int BeginIndex() const;
    int EndIndex() const;
    int Count() const; // the number of messages that are available
    Message operator[](int i) const;

    // the maximum number of messages to keep, 0 is unlimited. The oldest messages are removed in blocks, so up to a block of messages
    // more than this can be kept, but never less (if there are as many).
    int GetHistorySize() const;
    void SetHistorySize(int size);

private:
    struct InternalMessage
    {
        InternalMessage(double time, FILETIME systemTime, DWORD uid) :
            time(time),
            systemTime(systemTime),
            uid(uid)
        {
        }

        double time;
        FILETIME systemTime;
        DWORD uid;
    };

    void RemoveOldMessages();

    std::vector<InternalMessage> m_messages;
    int m_firstIndex = 0; // the index of m_messages[0], the messages before it were removed
    ProcessInfo m_processInfo;
    mutable indexedstorage::SnappyStorage m_storage;
    //    indexedstorage::VectorStorage m_storage;
    int m_historySize = 0;
};

} // namespace debugviewpp
} // namespace fusion
