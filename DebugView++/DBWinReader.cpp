// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DBWinBuffer.h"
#include "DBWinReader.h"
#include "ProcessInfo.h"

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

DBWinReader::DBWinReader(bool global) :
	m_autoNewLine(true),
	m_end(false),
	m_hBuffer(CreateDBWinBufferMapping(global)),
	m_dbWinBufferReady(CreateEvent(nullptr, false, true, GetDBWinName(global, L"DBWIN_BUFFER_READY").c_str())),
	m_dbWinDataReady(CreateEvent(nullptr, false, false, GetDBWinName(global, L"DBWIN_DATA_READY").c_str())),
	m_mappedViewOfFile(m_hBuffer.get(), PAGE_READONLY, 0, 0, sizeof(DbWinBuffer)),
	m_dbWinBuffer(static_cast<const DbWinBuffer*>(m_mappedViewOfFile.Ptr())),
	m_thread(&DBWinReader::Run, this),
	m_handleCacheTime(0.0)
{
	m_lines.reserve(4000);
	m_backBuffer.reserve(4000);
	SetEvent(m_dbWinBufferReady.get());
}

DBWinReader::~DBWinReader()
{
	Abort();
}

bool DBWinReader::AtEnd() const
{
	return false;
}

HANDLE DBWinReader::GetHandle() const 
{
	return m_dbWinDataReady.get();
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

Line DBWinReader::GetLine()
{
	HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, m_dbWinBuffer->processId);

#ifdef OPENPROCESS_DEBUG
	if (handle == 0)
	{
		Win32Error error(GetLastError(), "OpenProcess");
		std::string s = stringbuilder() << error.what() << " " <<  m_dbWinBuffer->data;
		Add(m_dbWinBuffer->processId, s.c_str(), handle);
		continue;
	}
#endif
	Add(m_dbWinBuffer->processId, m_dbWinBuffer->data, handle);
	SetEvent(m_dbWinBufferReady.get());

	Line line;
	return line;
}

void DBWinReader::Run()
{
	for (;;)
	{
		SetEvent(m_dbWinBufferReady.get());
		WaitForSingleObject(m_dbWinDataReady.get());
		if (m_end)
			break;

		HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, m_dbWinBuffer->processId);

#ifdef OPENPROCESS_DEBUG
		if (handle == 0)
		{
			Win32Error error(GetLastError(), "OpenProcess");
			std::string s = stringbuilder() << error.what() << " " <<  m_dbWinBuffer->data;
			Add(m_dbWinBuffer->processId, s.c_str(), handle);
			continue;
		}
#endif
		Add(m_dbWinBuffer->processId, m_dbWinBuffer->data, handle);
	}
}

void DBWinReader::Add(DWORD pid, const char* text, HANDLE handle)
{
	InternalLine line;
	line.time = m_timer.Get();
	line.systemTime = GetSystemTimeAsFileTime();
	line.pid = pid;
	line.handle = handle;
	line.message = text;
	AddLine(line);
}

void DBWinReader::AddLine(const InternalLine& InternalLine)
{
	boost::unique_lock<boost::mutex> lock(m_linesMutex);
	m_lines.push_back(InternalLine);
}

Lines DBWinReader::GetLines()
{
	m_backBuffer.clear();
	{
		boost::unique_lock<boost::mutex> lock(m_linesMutex);
		m_lines.swap(m_backBuffer);
	}
	return ProcessLines(m_backBuffer);
}

Lines DBWinReader::ProcessLines(const InternalLines& internalLines)
{
	Lines resolvedLines = CheckHandleCache();
	for (auto i = internalLines.begin(); i != internalLines.end(); ++i)
	{
		std::string processName; 
		if (i->handle)
		{
			Handle processHandle(i->handle);
			processName = Str(ProcessInfo::GetProcessName(processHandle.get())).str();
			AddCache(i->pid, std::move(processHandle));
		}

		Line line;
		line.time = i->time;
		line.systemTime = i->systemTime;
		line.pid = i->pid;
		line.processName = processName;
		line.message = i->message;

		auto lines = ProcessLine(line);
		for (auto line = lines.begin(); line != lines.end(); ++line)
			resolvedLines.push_back(*line);
	}

	return resolvedLines;
}

Lines DBWinReader::ProcessLine(const Line& line)
{
	Lines lines;
	if (m_lineBuffers.find(line.pid) == m_lineBuffers.end())
	{
		std::string message;
		message.reserve(4000);
		m_lineBuffers[line.pid] = std::move(message);
	}
	std::string& message = m_lineBuffers[line.pid];

	Line outputLine = line;
	for (auto i = line.message.begin(); i != line.message.end(); i++)
	{
		if (*i == '\r')
			continue;

		if (*i == '\n')
		{
			outputLine.message = std::move(message);
			message.clear();
			lines.push_back(outputLine);
		}
		else
		{
			message.push_back(char(*i));
		}
	}

	if (message.empty())
	{
		m_lineBuffers.erase(line.pid);
	}
	else if (m_autoNewLine || message.size() > 8192)	// 8k line limit prevents stack overflow in handling code 
	{
		outputLine.message = std::move(message);
		message.clear();
		lines.push_back(outputLine);
	}
	return lines;
}

void DBWinReader::AddCache(DWORD pid, Handle handle)
{
	if (m_handleCache.find(pid) == m_handleCache.end())
	{
		m_handleCache[pid] = std::move(handle);
	}
}

Lines DBWinReader::CheckHandleCache()
{
	std::vector<DWORD> removePids;
	Lines lines;
	if ((m_timer.Get() - m_handleCacheTime) > HandleCacheTimeout)
	{
		for (auto i = m_handleCache.begin(); i != m_handleCache.end(); i++)
		{
			DWORD pid = i->first;
			if (m_lineBuffers.find(pid) != m_lineBuffers.end())
			{
				DWORD exitcode = 0;
				BOOL result = GetExitCodeProcess(i->second.get(), &exitcode);
				if (result == FALSE || exitcode != STILL_ACTIVE)
				{
					if (!m_lineBuffers[pid].empty())
					{
						Line line;
						line.pid = pid;
						line.processName = "<flush>";
						line.time = m_timer.Get();
						line.systemTime = GetSystemTimeAsFileTime();
						line.message = m_lineBuffers[pid];
						lines.push_back(line);
					}
					m_lineBuffers.erase(pid);
					removePids.push_back(pid);
				}
			}
		}
		m_handleCacheTime = m_timer.Get();
	}

	for (auto i = removePids.begin(); i != removePids.end(); i++)
	{
		m_handleCache.erase(*i);
	}
	return lines;
}

} // namespace debugviewpp 
} // namespace fusion
