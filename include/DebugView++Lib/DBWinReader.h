// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <boost/utility.hpp>
#include <boost/thread.hpp>
#include <boost/signals2.hpp>

#include "LogSource.h"
#include "DBWinBuffer.h"
#include "ProcessHandleCache.h"
#include "Win32Lib/utilities.h"

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

class DBWinReader : public LogSource
{
public:
	explicit DBWinReader(ILineBuffer& linebuffer, bool global);
	virtual ~DBWinReader();
	virtual void Abort();
	virtual HANDLE GetHandle() const;
	virtual void Notify();

private:
	bool m_end;
	Handle m_hBuffer;
	Handle m_dbWinBufferReady;
	Handle m_dbWinDataReady;
	MappedViewOfFile m_mappedViewOfFile;
	const DbWinBuffer* m_dbWinBuffer;
};

} // namespace debugviewpp 
} // namespace fusion
