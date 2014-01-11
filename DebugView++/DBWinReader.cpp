// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DBWinReader.h"
#include "ProcessInfo.h"

namespace fusion {
namespace debugviewpp {

const double HandleCacheTimeout = 15.0; //seconds

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

bool IsDBWinViewerActive()
{
	Handle hMap(OpenFileMapping(FILE_MAP_READ, false, L"DBWIN_BUFFER"));
	return hMap != nullptr;
}

bool HasGlobalDBWinReaderRights()
{
	Handle hMap(::CreateFileMapping(nullptr, nullptr, PAGE_READWRITE, 0, sizeof(DbWinBuffer), L"Global\\DBWIN_BUFFER"));
	return hMap != nullptr;
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
	m_thread(boost::thread(&DBWinReader::Run, this)),
	m_handleCacheTime(0.0)
{
	m_lines.reserve(4000);
	m_backBuffer.reserve(4000);
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

#ifdef OPENPROCESS_DEBUG
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
	line.handle = handle;
	line.message = text;
	AddLine(line);
}

Lines DBWinReader::GetLines()
{
	m_backBuffer.clear();
	{
		boost::unique_lock<boost::mutex> lock(m_linesMutex);
		m_lines.swap(m_backBuffer);
	}
	return std::move(ProcessLines(m_backBuffer));
}

Lines DBWinReader::ProcessLines(const InternalLines& internalLines)
{
	Lines resolvedLines = CheckHandleCache();
	for (auto i = internalLines.begin(); i != internalLines.end(); ++i)
	{
		std::string processName; 
		if (i->handle)
		{
			AddCache(i->handle);
			processName = Str(ProcessInfo::GetProcessName(i->handle)).c_str();
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
	return std::move(lines);
}

void DBWinReader::AddCache(HANDLE handle)
{
	//todo: do not store multiple handles to the same process
	m_handleCache.push_back(std::move(Handle(handle)));
}

Lines DBWinReader::CheckHandleCache()
{
	Lines lines;
	if ((m_timer.Get() - m_handleCacheTime) > HandleCacheTimeout)
	{
		for (auto i = m_handleCache.begin(); i != m_handleCache.end(); i++)
		{
			DWORD pid = GetProcessId(i->get());
			if (m_lineBuffers.find(pid) != m_lineBuffers.end())
			{
				DWORD exitcode = 0;
				BOOL result = GetExitCodeProcess(i->get(), &exitcode);
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
				}
			}
		}
		m_handleCache.clear();
		m_handleCacheTime = m_timer.Get();
	}
	return std::move(lines);
}

DBWinWriter::DBWinWriter() :
	m_hBuffer(OpenFileMapping(FILE_MAP_WRITE, false, L"DBWIN_BUFFER")),
	m_dbWinBufferReady(OpenEvent(SYNCHRONIZE, false, L"DBWIN_BUFFER_READY")),
	m_dbWinDataReady(OpenEvent(EVENT_MODIFY_STATE, false, L"DBWIN_DATA_READY")),
	m_dbWinView(m_hBuffer.get(), FILE_MAP_WRITE, 0, 0, sizeof(DbWinBuffer))
{
}

void DBWinWriter::Write(DWORD pid, const std::string& message)
{
	if (!WaitForSingleObject(m_dbWinBufferReady.get(), 10000))
		return;

	auto pData = static_cast<DbWinBuffer*>(m_dbWinView.Ptr());
	pData->processId = pid;
	int length = std::min<int>(message.size(), sizeof(pData->data) - 1);
	std::copy(message.data(), message.data() + length, pData->data);
	pData->data[length] = '\0';

	SetEvent(m_dbWinDataReady.get());
}

PipeReader::PipeReader(HANDLE hPipe) :
	m_hPipe(hPipe),
	m_pid(GetParentProcessId()),
	m_process(Str(ProcessInfo::GetProcessName(m_pid)).str())
{
}

Line PipeReader::MakeLine(const std::string& text) const
{
	Line line;
	line.time = m_timer.Get();
	line.systemTime = GetSystemTimeAsFileTime();
	line.pid = m_pid;
	line.processName = m_process;
	line.message = text;
	return line;
}

Lines PipeReader::GetLines()
{
	if (!m_hPipe)
		return Lines();

	Lines lines;
	char buf[4096];
	char* start = std::copy(m_buffer.data(), m_buffer.data() + m_buffer.size(), buf);

	for (;;)
	{
		DWORD avail = 0;
		if (!PeekNamedPipe(m_hPipe, nullptr, 0, nullptr, &avail, nullptr))
		{
			CloseHandle(m_hPipe);
			break;
		}
		if (avail == 0)
			break;

		DWORD size = buf + sizeof(buf) - start;
		DWORD read;
		ReadFile(m_hPipe, start, size, &read, nullptr);

		char* begin = buf;
		char* end = start + read;
		char* p = start;
		while (p != end)
		{
			if (*p == '\0' || *p == '\n' || p - begin > 4000)
			{
				lines.push_back(MakeLine(std::string(begin, p)));
				begin = p + 1;
			}
			++p;
		}
		start = std::copy(begin, end, buf);
	}
	m_buffer = std::string(buf, start);
	return lines;
}

} // namespace debugviewpp 
} // namespace fusion
