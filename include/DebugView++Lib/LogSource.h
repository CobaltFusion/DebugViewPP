// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#pragma once

#include "DebugView++Lib/Line.h"
#include "DebugView++Lib/SourceType.h"
#include "Win32/Utilities.h"
#include "CobaltFusion/Timer.h"
#include "CobaltFusion/dbgstream.h"
#include "CobaltFusion/noncopyable.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer;
class LogSource : public fusion::noncopyable
{
public:
    LogSource(Timer& timer, SourceType::type sourceType, ILineBuffer& linebuffer);
    virtual ~LogSource();

    virtual void SetAutoNewLine(bool value);
    virtual bool GetAutoNewLine() const;

    // maybe called multiple times, the derived class is responsible for
    // executing initialization code once if needed.
    virtual void Initialize();

    virtual void Abort();

    // when true is returned, the Logsource is removed from LogSources and then destroyed.
    virtual bool AtEnd() const;

    // return a handle to wait for, Notify() is called when the handle is signaled. return INVALID_HANDLE_VALUE if the Logsource does not need to be notified
    virtual HANDLE GetHandle() const = 0;

    // only when nofity is called LogSource::Add may be used to add lines to the LineBuffer
    virtual void Notify() = 0;

    // called for each line before it is added to the view,
    // typically used to set the processname
    virtual void PreProcess(Line& line) const;

    std::wstring GetDescription() const;
    void SetDescription(const std::wstring& description);

    SourceType::type GetSourceType() const;

    // for DBWIN messages
    void Add(HANDLE handle, const std::string& message) const;

    // for Loopback messages and DBWIN kernel message that have no PID
    void Add(DWORD pid, const std::string& processName, const std::string& message);

    // used when reading from files
    void Add(double time, FILETIME systemTime, DWORD pid, const std::string& processName, const std::string& message);

    // used by Loopback and PolledLogSources writing internal status messages
    void AddInternal(const std::string& message) const;

    // used by FileReader
    void Add(const std::string& message);

private:
    bool m_autoNewLine = true;
    ILineBuffer& m_linebuffer;
    std::wstring m_description;
    SourceType::type m_sourceType;
    Timer& m_timer;
    bool m_end = false;
};

} // namespace debugviewpp
} // namespace fusion
