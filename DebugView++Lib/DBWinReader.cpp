// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DebugView++Lib/DBWinBuffer.h"
#include "DebugView++Lib/DBWinReader.h"
#include "DebugView++Lib/ProcessInfo.h"
#include "DebugView++Lib/LineBuffer.h"

namespace fusion {
namespace debugviewpp {

const double HandleCacheTimeout = 15.0; //seconds

std::wstring GetDBWinName(bool global, const std::wstring& name)
{
	return global ? L"Global\\" + name : name;
}

Handle CreateDBWinBufferMapping(bool global)
{
	Handle hMap(CreateFileMapping(nullptr, nullptr, PAGE_READWRITE, 0, sizeof(DbWinBuffer), GetDBWinName(global, L"DBWIN_BUFFER").c_str()));
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		throw std::runtime_error("CreateDBWinBufferMapping");
	return hMap;
}

DBWinReader::DBWinReader(Timer& timer, ILineBuffer& linebuffer, bool global) :
	LogSource(timer, SourceType::System, linebuffer),
	m_hBuffer(CreateDBWinBufferMapping(global)),
	m_dbWinBufferReady(CreateEvent(nullptr, false, true, GetDBWinName(global, L"DBWIN_BUFFER_READY").c_str())),
	m_dbWinDataReady(CreateEvent(nullptr, false, false, GetDBWinName(global, L"DBWIN_DATA_READY").c_str())),
	m_mappedViewOfFile(m_hBuffer.get(), PAGE_READONLY, 0, 0, sizeof(DbWinBuffer)),
	m_dbWinBuffer(static_cast<const DbWinBuffer*>(m_mappedViewOfFile.Ptr()))
{
	SetDescription(global ? L"Global Win32 Messages" : L"Win32 Messages");
	SetEvent(m_dbWinBufferReady.get());
}

DBWinReader::~DBWinReader()
{
}

HANDLE DBWinReader::GetHandle() const 
{
	return m_dbWinDataReady.get();
}

void DBWinReader::Notify()
{
	HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, m_dbWinBuffer->processId);

#ifdef OPENPROCESS_DEBUG
	if (handle == 0)
	{
		Win32Error error(GetLastError(), "OpenProcess");
		LogSource::Add(stringbuilder() << error.what() << ", data: " <<  m_dbWinBuffer->data << " (pid: " << m_dbWinBuffer->processId << ")");
	}
#endif
	// performance does not improve significantly (almost immeasurable) without the GetSystemTimeAsFileTime call, not without the OpenProcess call.
	LogSource::Add(m_dbWinBuffer->data, handle);
	SetEvent(m_dbWinBufferReady.get());
}

} // namespace debugviewpp 
} // namespace fusion
