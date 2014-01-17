// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "FileReader.h"
#include "Utilities.h"
#include <boost/filesystem.hpp>

namespace fusion {
namespace debugviewpp {

FileReader::FileReader(const std::wstring& filename) :
	m_end(false),
	m_filename(filename),
	m_handle(FindFirstChangeNotification( boost::filesystem::wpath(m_filename).parent_path().string().c_str(), FALSE, FILE_NOTIFY_CHANGE_SIZE)),
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
	SetEvent(m_handle);
	m_thread.join();
}


bool FileReader::AtEnd() const
{
	return false;
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
			WaitForSingleObject(m_handle);
			FindNextChangeNotification(m_handle);
		}
	}
	Add(stringbuilder() << "Stopped tailing " << Str(m_filename));
	FindCloseChangeNotification(m_handle);
	m_handle = 0;
}

void FileReader::Add(const std::string& text)
{
	Line line;
	line.time = m_timer.Get();
	line.systemTime = GetSystemTimeAsFileTime();
	line.pid = 0;
	line.message = text;
	line.processName = Str(boost::filesystem::wpath(m_filename).filename()).str();
	boost::unique_lock<boost::mutex> lock(m_linesMutex);
	m_buffer.push_back(line);
}

Lines FileReader::GetLines()
{
	Lines lines;
	{
		boost::unique_lock<boost::mutex> lock(m_linesMutex);
		m_buffer.swap(lines);
	}
	return std::move(lines);
}

} // namespace debugviewpp 
} // namespace fusion
