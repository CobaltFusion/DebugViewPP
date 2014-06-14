// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "DebugView++Lib/Line.h"
#include "DebugView++Lib/SourceType.h"
#include "Win32Lib/utilities.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer;
class LogSource
{
public:
	LogSource(SourceType::type sourceType, ILineBuffer& linebuffer);
	virtual ~LogSource();
	
	virtual void SetAutoNewLine(bool value);
	virtual bool GetAutoNewLine() const;
	virtual bool AtEnd() const = 0;
	virtual HANDLE GetHandle() const = 0;
	virtual void Notify() = 0;
	virtual std::string GetProcessName(const Line& line) const;

	std::wstring GetDescription() const;
	void SetDescription(const std::wstring& description);
	SourceType::type GetSourceType() const;

	// for DBWIN messages
	void Add(const char* message, HANDLE handle = 0);

	// for Loopback message 
	void Add(DWORD pid, const char* processName, const char* message, LogSource* logsource);

	// used when reading from files
	void Add(double time, FILETIME systemTime, DWORD pid, const char* processName, const char* message, LogSource* logsource);

private:
	bool m_autoNewLine;
	ILineBuffer& m_linebuffer;
	std::wstring m_description;
	SourceType::type m_sourceType;
	Timer m_timer;
};

} // namespace debugviewpp 
} // namespace fusion
