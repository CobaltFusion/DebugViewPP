// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "CobaltFusion/Str.h"
#include "DebugViewppLib/PolledLogSource.h"
#include "DebugViewppLib/ProcessReader.h"
#include "DebugViewppLib/LineBuffer.h"

namespace fusion {
namespace debugviewpp {

ProcessReader::ProcessReader(Timer& timer, ILineBuffer& linebuffer, const std::wstring& pathName, const std::wstring& args) :
    PolledLogSource(timer, SourceType::Pipe, linebuffer, 40),
    m_process(pathName, args),
    m_stdout(timer, linebuffer, m_process.GetStdOut(), m_process.GetProcessId(), Str(m_process.GetName()).str() + ":stdout", 0),
    m_stderr(timer, linebuffer, m_process.GetStdErr(), m_process.GetProcessId(), Str(m_process.GetName()).str() + ":stderr", 0)
{
    SetDescription(m_process.GetName() + L" stdout/stderr");
    AddMessage(Win32::DuplicateHandle(m_process.GetProcessHandle()), "Started capturing output of stdout/stderr, press Enter in the log view to send an Enter to the stdin of the process");
    Signal();
    StartThread();
}

ProcessReader::~ProcessReader() = default;

void ProcessReader::Abort()
{
    AddMessage(m_process.GetProcessId(), Str(m_process.GetName()).str(), "<process reader aborted>");
    Signal();
    PolledLogSource::Abort();
}

bool ProcessReader::AtEnd() const
{
    return m_stdout.AtEnd() && m_stderr.AtEnd();
}

bool ProcessReader::SendEnter()
{
    // a single character, because for example 'pause' reads one character each time, so a single Enter ends a single pause
    static const char enter[] = "\n";
    DWORD written = 0;
    if (::WriteFile(m_process.GetStdIn(), enter, sizeof(enter) - 1, &written, nullptr) == 0)
    {
        return false; // the process ended (broken pipe) or does not read its stdin
    }

    AddMessage(m_process.GetProcessId(), Str(m_process.GetName()).str(), "<Enter sent to stdin>");
    Signal();
    return true;
}

void ProcessReader::Poll()
{
    m_stdout.Poll(*this);
    m_stderr.Poll(*this);
}

} // namespace debugviewpp
} // namespace fusion
