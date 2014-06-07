// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DebugView++Lib/ProcessReader.h"
#include "DebugView++Lib/LineBuffer.h"

namespace fusion {
namespace debugviewpp {

ProcessReader::ProcessReader(ILineBuffer& linebuffer, const std::wstring& pathName, const std::wstring& args) :
	LogSource(SourceType::Pipe, linebuffer),
	m_process(pathName, args),
	m_stdout(linebuffer, m_process.GetStdOut(), m_process.GetProcessId(), Str(m_process.GetName()).str() + ":stdout"),
	m_stderr(linebuffer, m_process.GetStdErr(), m_process.GetProcessId(), Str(m_process.GetName()).str() + ":stderr")
{
	SetDescription(m_process.GetName() + L" stdout/stderr");
}

HANDLE ProcessReader::GetHandle() const
{
	return 0;	// todo::implement
}

void ProcessReader::Notify()
{
	// add a line to CircularBuffer
}

bool ProcessReader::AtEnd() const
{
	return m_stdout.AtEnd() && m_stderr.AtEnd();
}

Lines ProcessReader::GetLines()
{
	Lines lines(m_stdout.GetLines());
	Lines stderrLines(m_stderr.GetLines());
	lines.insert(lines.end(), stderrLines.begin(), stderrLines.end());
	return lines;
}

} // namespace debugviewpp 
} // namespace fusion
