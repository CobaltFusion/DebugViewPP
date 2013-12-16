// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Win32Lib.h"
#include "DBWinReader.h"

namespace fusion {

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

Handle CreateDBWinBufferMapping(bool global)
{
	Handle hMap(CreateFileMapping(nullptr, nullptr, PAGE_READWRITE, 0, sizeof(DbWinBuffer), GetDBWinName(global, L"DBWIN_BUFFER").c_str()));
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		throw std::runtime_error("Another DebugView is running");
	return hMap;
}

DBWinReader::DBWinReader(bool global) :
	m_autoNewLine(true),
	m_end(false),
	m_hBuffer(CreateDBWinBufferMapping(global)),
	m_dbWinBufferReady(CreateEvent(nullptr, false, true, GetDBWinName(global, L"DBWIN_BUFFER_READY").c_str())),
	m_dbWinDataReady(CreateEvent(nullptr, false, false, GetDBWinName(global, L"DBWIN_DATA_READY").c_str())),
	m_thread(boost::thread(&DBWinReader::Run, this))
{
	m_lines.reserve(4000);
	m_linesBackingBuffer.reserve(4000);
}

DBWinReader::~DBWinReader()
{
	Abort();
}

bool DBWinReader::AutoNewLine() const
{
	return m_autoNewLine;
}

void DBWinReader::AutoNewLine(bool value)
{
	m_autoNewLine = value;
}

void DBWinReader::Abort()
{
	m_end = true;
	SetEvent(m_dbWinDataReady.get());	// will this not interfere with other DBWIN listers? There can be only one DBWIN client..
	m_thread.join();
}

void DBWinReader::Run()
{
	MappedViewOfFile dbWinView(m_hBuffer.get(), PAGE_READONLY, 0, 0, sizeof(DbWinBuffer));
	auto pData = static_cast<const DbWinBuffer*>(dbWinView.Ptr());

	for (;;)
	{
		SetEvent(m_dbWinBufferReady.get());
		WaitForSingleObject(m_dbWinDataReady.get());
		if (m_end)
			break;

		Add(pData->processId, pData->data);
	}
}

void DBWinReader::AddLine(const Line& line)
{
	boost::unique_lock<boost::mutex> lock(m_linesMutex);
	m_lines.push_back(line);
}

void DBWinReader::Add(DWORD pid, const char* text)
{
	Line line;
	line.time = m_timer.Get();
	line.systemTime = GetSystemTimeAsFileTime();
	line.pid = pid;
	line.message.reserve(4000);

	if (m_lineBuffer.message.empty())
		m_lineBuffer = line;

	while (*text)
	{
		if (*text == '\n')
		{
			AddLine(m_lineBuffer);
			m_lineBuffer = line;
		}
		else
		{
			m_lineBuffer.message.push_back(*text);
		}
		++text;
	}

	if (!m_lineBuffer.message.empty() && m_autoNewLine)
	{
		AddLine(m_lineBuffer);
		m_lineBuffer.message.clear();
	}
}

const Lines& DBWinReader::GetLines()
{
	m_linesBackingBuffer.clear();
	{
		boost::unique_lock<boost::mutex> lock(m_linesMutex);
		m_lines.swap(m_linesBackingBuffer);
	}
	return m_linesBackingBuffer;
}

} // namespace fusion
