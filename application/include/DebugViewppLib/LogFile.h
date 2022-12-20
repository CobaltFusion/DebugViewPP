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
    int BeginIndex() const;
    int EndIndex() const;
    int Count() const;
    Message operator[](int i) const;
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

    std::vector<InternalMessage> m_messages;
    ProcessInfo m_processInfo;
    mutable indexedstorage::SnappyStorage m_storage;
    //    indexedstorage::VectorStorage m_storage;
    int m_historySize = 0;
};

} // namespace debugviewpp
} // namespace fusion
