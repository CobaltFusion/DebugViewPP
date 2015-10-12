// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "LogSource.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

struct DBWinMessage
{
	double time;
	FILETIME systemTime;
	DWORD pid;
	std::string message;
	HANDLE handle;
};

typedef std::vector<DBWinMessage> DBWinMessages;

typedef void OnDBWinMessage(double time, FILETIME systemTime, DWORD processId, HANDLE processHandle, const char* message);

struct DbWinBuffer;

class DBWinReader : public LogSource
{
public:
	DBWinReader(Timer& timer, ILineBuffer& linebuffer, bool global);
	virtual ~DBWinReader();
	virtual HANDLE GetHandle() const;
	virtual void Notify();

private:
	bool m_end;
	Win32::Handle m_hBuffer;
	Win32::Handle m_dbWinBufferReady;
	Win32::Handle m_dbWinDataReady;
	Win32::MappedViewOfFile m_mappedViewOfFile;
	const DbWinBuffer* m_dbWinBuffer;
};

} // namespace debugviewpp 
} // namespace fusion
