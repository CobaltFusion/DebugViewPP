// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "ProcessReader.h"

namespace fusion {
namespace debugviewpp {

ProcessReader::ProcessReader(const std::wstring& pathName, const std::wstring& args) :
	m_process(pathName, args),
	m_stdout(m_process.GetStdOut(), m_process.GetProcessId(), Str(m_process.GetName()).str() + ":stdout"),
	m_stderr(m_process.GetStdErr(), m_process.GetProcessId(), Str(m_process.GetName()).str() + ":stderr")
{
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
