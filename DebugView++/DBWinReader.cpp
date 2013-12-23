// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Win32Lib.h"
#include "DBWinReader.h"
#include "ProcessInfo.h"

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

		HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pData->processId);

#define DEBUG_OPENPROCESS
#ifdef DEBUG_OPENPROCESS
		if (handle == 0)
		{
			Win32Error error(GetLastError(), "OpenProcess");
			std::string s = stringbuilder() << error.what() << " " <<  pData->data;
			Add(pData->processId, s.c_str(), handle);
			continue;
		}
#endif
		Add(pData->processId, pData->data, handle);
	}
}

void DBWinReader::AddLine(const InternalLine& InternalLine)
{
	boost::unique_lock<boost::mutex> lock(m_linesMutex);
	m_lines.push_back(InternalLine);
}

void DBWinReader::Add(DWORD pid, const char* text, HANDLE handle)
{
	InternalLine line;
	line.time = m_timer.Get();
	line.systemTime = GetSystemTimeAsFileTime();
	line.pid = pid;
	line.message.reserve(4000);
	line.handle = handle;

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

	if (!m_lineBuffer.message.empty() && (m_autoNewLine || m_lineBuffer.message.size() > 8192))	// 8k InternalLine limit prevents stack overflows in handling code 
	{
		AddLine(m_lineBuffer);
		m_lineBuffer.message.clear();
	}
}

Lines DBWinReader::GetLines()
{
	InternalLines lines;
	lines.reserve(4000);
	{
		boost::unique_lock<boost::mutex> lock(m_linesMutex);
		m_lines.swap(lines);
	}
	return std::move(ResolveLines(lines));
}

Lines DBWinReader::ResolveLines(const InternalLines& internalLines)
{
	Lines resolvedLines;
	for (auto i=internalLines.begin(); i != internalLines.end(); i++)
	{
		Line line;
		line.time = i->time;
		line.systemTime = i->systemTime;
		line.pid = i->pid;
		line.message = i->message;
		if (i->handle)
		{
			Handle handle(i->handle);
			line.processName = Str(ProcessInfo::GetProcessName(handle.get())).c_str();
		}
		resolvedLines.push_back(line);
	}
	return std::move(resolvedLines);
}

} // namespace fusion
