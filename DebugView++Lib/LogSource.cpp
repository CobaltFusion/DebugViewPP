// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "stdafx.h"
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

bool LogSource::AtEnd() const
{
	return m_end;
}

void LogSource::SetAutoNewLine(bool value)
{
	m_autoNewLine = value;
}

bool LogSource::GetAutoNewLine() const
{
	return m_autoNewLine;
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

void LogSource::Add(double time, FILETIME systemTime, DWORD pid, const char* processName, const char* message, LogSource* logsource)
{
	m_linebuffer.Add(time, systemTime, pid, processName, message, logsource);
}

void LogSource::Add(DWORD pid, const char* processName, const char* message, LogSource* logsource)
{
	m_linebuffer.Add(m_timer.Get(), GetSystemTimeAsFileTime(), pid, processName, message, logsource);
}

void LogSource::Add(const char* message, HANDLE handle)
{
	m_linebuffer.Add(m_timer.Get(), GetSystemTimeAsFileTime(), handle, message, this);
}

void LogSource::PreProcess(Line& line) const
{
	if (line.handle != 0)
	{
		line.processName = Str(ProcessInfo::GetProcessName(line.handle)).str();
	}
}

void LogSource::Abort()
{
	m_end = true;
}

} // namespace debugviewpp 
} // namespace fusion
