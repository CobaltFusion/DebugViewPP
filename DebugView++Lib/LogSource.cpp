// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "CobaltFusion/Str.h"
#include "DebugView++Lib/LogSource.h"
#include "DebugView++Lib/LineBuffer.h"
#include "DebugView++Lib/ProcessInfo.h"

namespace fusion {
namespace debugviewpp {

LogSource::LogSource(Timer& timer, SourceType::type sourceType, ILineBuffer& linebuffer) :
	m_timer(timer),
	m_sourceType(sourceType),
	m_linebuffer(linebuffer),
	m_autoNewLine(true),
	m_end(false)
{
}

LogSource::~LogSource()
{
}

void LogSource::SetAutoNewLine(bool value)
{
	m_autoNewLine = value;
}

bool LogSource::GetAutoNewLine() const
{
	return m_autoNewLine;
}

void LogSource::Initialize()
{
}

void LogSource::Abort()
{
	m_end = true;
}

bool LogSource::AtEnd() const
{
	return m_end;
}

void LogSource::PreProcess(Line& line) const
{
	if (line.handle)
		line.processName = Str(ProcessInfo::GetProcessName(line.handle)).str();
}

std::wstring LogSource::GetDescription() const
{
	return m_description;
}

void LogSource::SetDescription(const std::wstring& description)
{
	m_description = description;
}

SourceType::type LogSource::GetSourceType() const
{
	return m_sourceType;
}

void LogSource::Add(double time, FILETIME systemTime, DWORD pid, const std::string& processName, const std::string& message)
{
	m_linebuffer.Add(time, systemTime, pid, processName, message, this);
}

void LogSource::Add(DWORD pid, const std::string& processName, const std::string& message)
{
	m_linebuffer.Add(m_timer.Get(), Win32::GetSystemTimeAsFileTime(), pid, processName, message, this);
}

void LogSource::Add(HANDLE handle, const std::string& message) const
{
	m_linebuffer.Add(m_timer.Get(), Win32::GetSystemTimeAsFileTime(), handle, message, this);
}

void LogSource::Add(const std::string& message)
{
	m_linebuffer.Add(m_timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "", message, this);
}

void LogSource::AddInternal(const std::string& message) const
{
	m_linebuffer.Add(m_timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "[internal]", message, this);
}

} // namespace debugviewpp 
} // namespace fusion
