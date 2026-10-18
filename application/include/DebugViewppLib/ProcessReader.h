// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "PipeReader.h"
#include "PolledLogSource.h"
#include "Win32/Process.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

class ProcessReader : public PolledLogSource
{
public:
    ProcessReader(Timer& timer, ILineBuffer& linebuffer, const std::wstring& pathName, const std::wstring& args);
    ~ProcessReader() override;
    void Abort() override;
    bool AtEnd() const override;

    // Writes an Enter (a single line feed) to the stdin of the process, for programs and batch files that wait for it (like 'pause'). Returns false if
    // that is not possible, because the process ended for example.
    bool SendEnter();

private:
    void Poll() override;

    Win32::Process m_process;
    PipeReader m_stdout;
    PipeReader m_stderr;
};

} // namespace debugviewpp
} // namespace fusion
