// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <boost/filesystem.hpp>
#include "DebugView++Lib/FileIO.h"
#include "DebugView++Lib/FileReader.h"

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

void FileReader::Notify()
{
	// add a line to CircularBuffer
}

void FileReader::Run()
{
	std::ifstream ifs(m_filename);
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
	Line line(m_timer.Get(), GetSystemTimeAsFileTime(), 0, boost::filesystem::wpath(m_filename).filename().string(), text);
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

void DBLogReader::Notify()
{
	// add a line to CircularBuffer
}

void DBLogReader::Add(const std::string& data)
{
	Line line = Line();
	line.systemTime = m_time;
	line.processName = m_name;
	ReadLogFileMessage(data, line);
	m_buffer.push_back(line);
}

} // namespace debugviewpp 
} // namespace fusion
