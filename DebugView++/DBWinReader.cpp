//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://boosttestui.wordpress.com/ for the boosttestui home page.

#include "stdafx.h"
#include "Win32Lib.h"
#include "DBWinReader.h"

namespace gj {

struct DbWinBuffer
{
	DWORD processId;
	// Total size must be 4KB (processID + data)
	char data[4096 - sizeof(DWORD)];
};

static_assert(sizeof(DbWinBuffer) == 4096, "DBWIN_BUFFER size must be 4096");

std::wstring GetDBWinName(bool global, const std::wstring& name)
{
	return global ? L"Global\\" + name : name;
}

CHandle GetDBWinMutex(bool global)
{
	CHandle hMutex(OpenMutex(SYNCHRONIZE, FALSE, GetDBWinName(global, L"DBWinMutex").c_str()));
	if (!hMutex)
		hMutex = CreateMutex(nullptr, false, GetDBWinName(global, L"DBWinMutex").c_str());
	return hMutex;
}

DBWinReader::DBWinReader(bool global) :
	m_end(false)
	//bufferMutex(GetDBWinMutex(global))
{
//	MutexLock bufferLock(bufferMutex);

	hBuffer = CreateFileMapping(nullptr, nullptr, PAGE_READWRITE, 0, sizeof(DbWinBuffer), GetDBWinName(global, L"DBWIN_BUFFER").c_str());
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		throw std::runtime_error("Another DebugView is running");

	dbWinBufferReady = CreateEvent(nullptr, false, true, GetDBWinName(global, L"DBWIN_BUFFER_READY").c_str());
	dbWinDataReady = CreateEvent(nullptr, false, false, GetDBWinName(global, L"DBWIN_DATA_READY").c_str());

	m_thread = boost::thread(&DBWinReader::Run, this);
}

DBWinReader::~DBWinReader()
{
	Abort();

//	MutexLock bufferLock(bufferMutex);

	dbWinDataReady.Close();
	dbWinBufferReady.Close();
	hBuffer.Close();
}

boost::signals2::connection DBWinReader::ConnectOnMessage(OnMessage::slot_type slot)
{
	return m_onMessage.connect(slot);
}

void DBWinReader::Abort()
{
	m_end = true;
	SetEvent(dbWinDataReady);
	m_thread.join();
}

void DBWinReader::Run()
{
	MappedViewOfFile dbWinView(hBuffer, PAGE_READONLY, 0, 0, sizeof(DbWinBuffer));
	auto pData = static_cast<const DbWinBuffer*>(dbWinView.Ptr());
	for (;;)
	{
		WaitForSingleObject(dbWinDataReady);

		if (m_end)
			break;
		m_onMessage(pData->processId, pData->data);
		SetEvent(dbWinBufferReady);
	}
	SetEvent(dbWinBufferReady);
}

} // namespace gj
