// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DebugView++Lib/PassiveLogSource.h"
#include "DebugView++Lib/ProcessReader.h"
#include "DebugView++Lib/LineBuffer.h"

namespace fusion {
namespace debugviewpp {

ProcessReader::ProcessReader(Timer& timer, ILineBuffer& linebuffer, const std::wstring& pathName, const std::wstring& args) :
	PassiveLogSource(timer, SourceType::Pipe, linebuffer, 40),
	m_process(pathName, args),
	m_stdout(timer, linebuffer, m_process.GetStdOut(), m_process.GetProcessId(), Str(m_process.GetName()).str() + ":stdout", 0),
	m_stderr(timer, linebuffer, m_process.GetStdErr(), m_process.GetProcessId(), Str(m_process.GetName()).str() + ":stderr", 0)
{
	SetDescription(m_process.GetName() + L" stdout/stderr");
}

ProcessReader::~ProcessReader()
{
}

bool ProcessReader::AtEnd() const
{
	return m_stdout.AtEnd() && m_stderr.AtEnd();
}

void ProcessReader::Poll()
{
	m_stdout.Poll(*this);
	m_stderr.Poll(*this);
}

} // namespace debugviewpp 
} // namespace fusion
