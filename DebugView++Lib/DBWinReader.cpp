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
#include "CobaltFusion/stringbuilder.h"

namespace fusion {
namespace debugviewpp {

const char* systemProcessNames[] = { "SYSTEM_PID_0", "SYSTEM_PID_1", "SYSTEM_PID_2", "SYSTEM_PID_3", "SYSTEM_PID_4" };
constexpr int systemProcessNamesCount = sizeof(systemProcessNames) / sizeof(systemProcessNames[0]);

std::wstring GetDBWinName(bool global, const std::wstring& name)
{
	return global ? L"Global\\" + name : name;
}

Win32::Handle CreateDBWinBufferMapping(bool global)
{
	Win32::Handle hMap(CreateFileMapping(nullptr, nullptr, PAGE_READWRITE, 0, sizeof(DbWinBuffer), GetDBWinName(global, L"DBWIN_BUFFER").c_str()));
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		throw std::runtime_error("CreateDBWinBufferMapping");
    return hMap;
}

DBWinReader::DBWinReader(Timer& timer, ILineBuffer& linebuffer, bool global) :
	LogSource(timer, SourceType::System, linebuffer),
	m_hBuffer(CreateDBWinBufferMapping(global)),

	m_dbWinBufferReady(Win32::CreateEvent(nullptr, false, true, GetDBWinName(global, L"DBWIN_BUFFER_READY").c_str())),
	m_dbWinDataReady(Win32::CreateEvent(nullptr, false, false, GetDBWinName(global, L"DBWIN_DATA_READY").c_str())),
	m_mappedViewOfFile(m_hBuffer.get(), PAGE_READONLY, 0, 0, sizeof(DbWinBuffer)),
	m_dbWinBuffer(static_cast<const DbWinBuffer*>(m_mappedViewOfFile.Ptr()))
{
	SetDescription(global ? L"Global Win32 Messages" : L"Win32 Messages");

	//Option 1:
	//Win32::AdjustObjectDACL(m_hBuffer.get());
	//Win32::AdjustObjectDACL(m_dbWinBufferReady.get());
	//Win32::AdjustObjectDACL(m_dbWinDataReady.get());

	//Option 2:
	//Win32::DeleteObjectDACL(m_hBuffer.get());
	//Win32::DeleteObjectDACL(m_dbWinBufferReady.get());
	//Win32::DeleteObjectDACL(m_dbWinDataReady.get());

	//TODO: Please test this and choose one

	Win32::SetEvent(m_dbWinBufferReady);
}

HANDLE DBWinReader::GetHandle() const 
{
	return m_dbWinDataReady.get();
}

// by forcing Notify() to be not inlined, it will have a better change of being aligned correctly
// in the instruction cache. __declspec(noinline), however measurements do not show any differences
void DBWinReader::Notify()
{
	static_assert(systemProcessNamesCount == Win32::fixedNumberOfSystemPids, "The size of the systemProcessNames array must be 'fixedNumberOfSystemPids' (5)");
	if (m_dbWinBuffer->processId < Win32::fixedNumberOfSystemPids)
	{
		Add(m_dbWinBuffer->processId, systemProcessNames[m_dbWinBuffer->processId], m_dbWinBuffer->data);
	}
	else
	{
		HANDLE handle = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, m_dbWinBuffer->processId);
#ifdef OPENPROCESS_DEBUG
		if (!handle)
		{
			Win32::Win32Error error(GetLastError(), "OpenProcess");
			LogSource::Add(stringbuilder() << error.what() << ", data: " <<  m_dbWinBuffer->data << " (pid: " << m_dbWinBuffer->processId << ")");
		}
#else
		// performance does not improve significantly (almost immeasurable) without the Add() call, nor without the OpenProcess call.
		// there might be performance to be gained by:
		// - making a non-virtual Notice() method
		// - copying the information from m_dbWinBuffer->data and calling SetEvent(m_dbWinBufferReady.get()); before _any_ other operation.
		// - check the m_dbWinDataReady is already set again after Add() 
		if (handle)
			Add(handle, m_dbWinBuffer->data);
		else
			Add(m_dbWinBuffer->processId, "<system>", m_dbWinBuffer->data);
#endif
	}
	::SetEvent(m_dbWinBufferReady.get());
}

} // namespace debugviewpp 
} // namespace fusion
