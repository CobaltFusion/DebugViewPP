// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <boost/filesystem.hpp>
#include "Utilities.h"
#include "FileReader.h"

namespace fusion {
namespace debugviewpp {

FileReader::FileReader(const std::wstring& filename) :
	m_end(false),
	m_filename(filename),
	m_name(Str(boost::filesystem::wpath(filename).filename().string()).str()),
	m_handle(FindFirstChangeNotification(boost::filesystem::wpath(m_filename).parent_path().wstring().c_str(), false, FILE_NOTIFY_CHANGE_SIZE)),
	m_thread(&FileReader::Run, this)
{
}

FileReader::~FileReader()
{
	Abort();
}

void FileReader::Abort()
{
	m_end = true;
	m_thread.join();
}

bool FileReader::AtEnd() const
{
	return false;
}

HANDLE FileReader::GetHandle() const
{
	return 0;	// todo::implement
}

Line FileReader::GetLine()
{
	Line line; // todo::implement
	return line;
}

void FileReader::Run()
{
	std::ifstream ifs(m_filename.c_str(), std::ifstream::in);
	if (ifs.is_open())
	{
		std::string line;
		for (;;)
		{
			while (std::getline(ifs, line)) 
				Add(line);
			if (!ifs.eof()) 
			{
				// Some error other then EOF occured
				break; 
			}
			ifs.clear(); // clear EOF condition
			bool signalled = WaitForSingleObject(m_handle.get(), 1000);
			if (m_end)
				break;
			if (signalled)
				FindNextChangeNotification(m_handle.get());
		}
	}
	Add(stringbuilder() << "Stopped tailing " << Str(m_filename));
}

void FileReader::Add(const std::string& text)
{
	Line line;
	line.time = m_timer.Get();
	line.systemTime = GetSystemTimeAsFileTime();
	line.pid = 0;
	line.message = text;
	line.processName = boost::filesystem::wpath(m_filename).filename().string();
	boost::unique_lock<boost::mutex> lock(m_linesMutex);
	m_buffer.push_back(line);
}

Lines FileReader::GetLines()
{
	Lines lines;
	boost::unique_lock<boost::mutex> lock(m_linesMutex);
	m_buffer.swap(lines);
	return lines;
}

DBLogReader::DBLogReader(const std::wstring& filename) : 
	FileReader(filename),
	m_time(GetSystemTimeAsFileTime())
{
}

HANDLE DBLogReader::GetHandle() const
{
	return 0;	// todo::implement
}

Line DBLogReader::GetLine()
{
	Line line; // todo::implement
	return line;
}

bool ReadTime(const std::string& s, double& time)
{
	std::istringstream is(s);
	return is >> time && is.eof();
}

bool ReadSystemTime(const std::string& text, const FILETIME& ftRef, FILETIME& ft)
{
	std::istringstream is(text);
	WORD h, m, s;
	char c1, c2, p1, p2;
	if (!((is >> h >> c1 >> m >> c2 >> s) && c1 == ':' && c2 == ':'))
		return false;
	if (is >> p1 >> p2 && p1 == 'P' && p2 == 'M')
		h += 12;

	SYSTEMTIME st = FileTimeToSystemTime(ftRef);
	st.wHour = h;
	st.wMinute = m;
	st.wSecond = s;
	st.wMilliseconds = 0;
	ft = SystemTimeToFileTime(st);
	return true;
}

void DBLogReader::Add(const std::string& text)
{
	TabSplitter split(text);
	auto col1 = split.GetNext();
	auto col2 = split.GetNext();
	auto col3 = split.GetNext();
	if (!col3.empty() && col3[0] == '[')
	{
		std::istringstream is3(col3);
		DWORD pid;
		char c1, c2, c3;
		std::string msg;
		if (is3 >> std::noskipws >> c1 >> pid >> c2 >> c3 && c1 == '[' && c2 == ']' && c3 == ' ' && std::getline(is3, msg))
		{
			Line line;
			if (ReadTime(col2, line.time))
			{
				line.systemTime = m_time;
			}
			else
			{
				line.time = 0;
				if (!ReadSystemTime(col2, m_time, line.systemTime))
					line.systemTime = m_time;
			}
			line.pid = pid;
			line.processName = Str(m_name).str();
			line.message = msg;
			m_buffer.push_back(line);
		}
	}
	else
	{
		Line line;
		line.time = boost::lexical_cast<double>(col1);
		line.systemTime = MakeFileTime(boost::lexical_cast<uint64_t>(col2));
		line.pid = boost::lexical_cast<DWORD>(col3);
		line.processName = split.GetNext();
		line.message = split.GetTail();
		m_buffer.push_back(line);
	}

	Line line;
	line.time = m_timer.Get();
	line.systemTime = GetSystemTimeAsFileTime();
	line.pid = 0;
	line.message = text;
	line.processName = boost::filesystem::wpath(m_filename).filename().string();
	boost::unique_lock<boost::mutex> lock(m_linesMutex);
	m_buffer.push_back(line);
}

} // namespace debugviewpp 
} // namespace fusion
