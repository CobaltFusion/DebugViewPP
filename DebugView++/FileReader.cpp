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
	m_filename(filename),
	m_handle(FindFirstChangeNotification(m_filename.c_str(), FALSE, FILE_NOTIFY_CHANGE_SIZE)),
	m_thread(&FileReader::Run, this)
{
}

bool FileReader::AtEnd() const
{
	return false;
}

void FileReader::Run()
{
	std::ifstream ifs(m_filename.c_str());
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
			WaitForSingleObject(m_handle.get());
			FindNextChangeNotification(m_handle.get());
		}
	}
	Add(stringbuilder() << "Stopped tailing " << Str(m_filename));
	FindCloseChangeNotification(m_handle.get());
	m_handle.reset();
}

void FileReader::Add(const std::string& text)
{
	Line line;
	line.time = m_timer.Get();
	line.systemTime = GetSystemTimeAsFileTime();
	line.pid = 0;
	line.processName = boost::filesystem::wpath(m_filename).filename().string();
	line.message = text;
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
