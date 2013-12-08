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

DBWinReader::DBWinReader(bool global) :
	m_end(false)
{
	m_hBuffer = CreateFileMapping(nullptr, nullptr, PAGE_READWRITE, 0, sizeof(DbWinBuffer), GetDBWinName(global, L"DBWIN_BUFFER").c_str());
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		throw std::runtime_error("Another DebugView is running");

	m_dbWinBufferReady = CreateEvent(nullptr, false, true, GetDBWinName(global, L"DBWIN_BUFFER_READY").c_str());
	m_dbWinDataReady = CreateEvent(nullptr, false, false, GetDBWinName(global, L"DBWIN_DATA_READY").c_str());
	m_thread = boost::thread(&DBWinReader::Run, this);
}

DBWinReader::~DBWinReader()
{
	Abort();
	m_dbWinDataReady.Close();
	m_dbWinBufferReady.Close();
	m_hBuffer.Close();
}

void DBWinReader::Abort()
{
	m_end = true;
	SetEvent(m_dbWinDataReady);	// will this not interfere with other DBWIN listers? There can be only one DBWIN client..
	m_thread.join();
}

void DBWinReader::Add(DWORD pid, const char* text)
{
	Line line;
	line.time = m_timer.Get();
	line.systemTime = GetSystemTimeAsFileTime();
	line.pid = pid;
	line.message = std::string(text);

	boost::unique_lock<boost::mutex> lock(m_linesMutex);
	m_lines.push_back(line);
}

void DBWinReader::Run()
{
	MappedViewOfFile dbWinView(m_hBuffer, PAGE_READONLY, 0, 0, sizeof(DbWinBuffer));
	auto pData = static_cast<const DbWinBuffer*>(dbWinView.Ptr());

	for (;;)
	{
		SetEvent(m_dbWinBufferReady);
		WaitForSingleObject(m_dbWinDataReady);
		if (m_end)
			break;

		Add(pData->processId, pData->data);
	}
}

LinesList DBWinReader::GetLines()
{
	boost::unique_lock<boost::mutex> lock(m_linesMutex);
	LinesList temp;
	temp.swap(m_lines);
	return std::move(temp);
}

} // namespace gj
