// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "DebugView++Lib/Line.h"
#include "DebugView++Lib/SourceType.h"
#include "Win32Lib/utilities.h"
#include "CobaltFusion/dbgstream.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer;
class LogSource : public boost::noncopyable, public std::enable_shared_from_this<LogSource>
{
public:
	LogSource(Timer& timer, SourceType::type sourceType, ILineBuffer& linebuffer);
	virtual ~LogSource();
	
	virtual void SetAutoNewLine(bool value);
	virtual bool GetAutoNewLine() const;

	// maybe called multiple times, the derived class is responsible for
	// executing initialization code once if needed.
	virtual void Initialize() {}

	virtual void Abort();

	// return when true is returned, the Logsource is removed from LogSources and usually the means it leave scope
	virtual bool AtEnd() const;

	// return a handle to wait for, Notify() is called when the handle is signaled
	virtual HANDLE GetHandle() const = 0;
	
	// only when nofity is called LogSource::Add may be used to add lines to the LineBuffer
	virtual void Notify() = 0;

	// called for each line before it is added to the view,
	// typically used to modify the processname for non-dbwin logsources
	virtual void PreProcess(Line& line) const;

	std::wstring GetDescription() const;
	void SetDescription(const std::wstring& description);

	SourceType::type GetSourceType() const;

	// for DBWIN messages
	void Add(const char* message, HANDLE handle = 0);

	// for Loopback messages
	void Add(DWORD pid, const char* processName, const char* message);

	// used when reading from files
	void Add(double time, FILETIME systemTime, DWORD pid, const char* processName, const char* message);

	// used by PassiveLogsources writing internal status messages
	void Add(const std::string& message);

private:
	bool m_autoNewLine;
	ILineBuffer& m_linebuffer;
	std::wstring m_description;
	SourceType::type m_sourceType;
	Timer& m_timer;
	bool m_end;
};

} // namespace debugviewpp 
} // namespace fusion
