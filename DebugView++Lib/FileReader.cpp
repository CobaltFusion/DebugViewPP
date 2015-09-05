// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <boost/filesystem.hpp>
#include "DebugView++Lib/FileIO.h"
#include "DebugView++Lib/FileReader.h"
#include "DebugView++Lib/LineBuffer.h"
#include "DebugView++Lib/Line.h"

namespace fusion {
namespace debugviewpp {

FileReader::FileReader(Timer& timer, ILineBuffer& linebuffer, FileType::type filetype, const std::wstring& filename) :
	LogSource(timer, SourceType::File, linebuffer),
	m_end(true),
	m_filename(Str(filename).str()),
	m_fileType(filetype),
	m_name(Str(boost::filesystem::wpath(filename).filename().string()).str()),
	m_handle(FindFirstChangeNotification(boost::filesystem::wpath(m_filename).parent_path().wstring().c_str(), false, FILE_NOTIFY_CHANGE_SIZE)), //todo: maybe use FILE_NOTIFY_CHANGE_LAST_WRITE ?
	m_ifstream(m_filename, std::ios::in),
	m_filenameOnly(boost::filesystem::wpath(m_filename).filename().string()),
	m_initialized(false)
{
	SetDescription(filename);
}

FileReader::~FileReader()
{
}

void FileReader::Initialize()
{
	if (m_initialized)
	{
		return;
	}
	m_initialized = true;

	if (m_ifstream.is_open())
	{
		ReadUntilEof();
		m_end = false;
	}
}

bool FileReader::AtEnd() const
{
	return m_end;
}

HANDLE FileReader::GetHandle() const
{
	return m_handle.get();
}

void FileReader::Notify()
{
	ReadUntilEof();
	FindNextChangeNotification(m_handle.get());
}

void FileReader::ReadUntilEof()
{
	std::string line;
	while (std::getline(m_ifstream, line))
		AddLine(line);

	if (m_ifstream.eof()) 
	{
		m_ifstream.clear(); // clear EOF condition

		// resync to end of file, even it the file shrunk
		auto lastReadPosition = m_ifstream.tellg();
		m_ifstream.seekg(0, m_ifstream.end);
		auto length = m_ifstream.tellg();
		if (length > lastReadPosition)
			m_ifstream.seekg(lastReadPosition);
		else if (length != lastReadPosition)
			Add(stringbuilder() << "file shrank, resynced at offset " << length);
	}
	else
	{
		// Some error other then EOF occured
		Add("Stopped tailing " + m_filename);
		m_end = true;
	}
}

void FileReader::AddLine(const std::string& line)
{
	Add(line.c_str());
}

void FileReader::PreProcess(Line& line) const
{
	line.processName = m_filenameOnly;
}

// todo: Reading support for more filetypes, maybe not, who logs in unicode anyway?
// posepone until we have a valid usecase
// ANSI/ASCII
// UTF-8
// UTF-16
// UTF-8 NO BOM ? 
// UTF-16 NO BOM ?
// UTF-16 Big Endian
// UTF-16 Big Endian NO BOM? 
// Unicode ASCII escaped.

// http://stackoverflow.com/questions/10504044/correctly-reading-a-utf-16-text-file-into-a-string-without-external-libraries

// check a utf-16 file will always contain an even-amount of bytes?
// std::wifstream ifs(m_filename, std::ios::binary);
// fin.imbue(std::locale(fin.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>));
// for(wchar_t c; fin.get(c); ) std::cout << std::showbase << std::hex << c << '\n';


DBLogReader::DBLogReader(Timer& timer, ILineBuffer& linebuffer, FileType::type filetype, const std::wstring& filename) : 
	FileReader(timer, linebuffer, filetype, filename),
	m_linenumber(0)
{
}

double GetDifference(FILETIME ft1, FILETIME ft2)
{
	return (*((ULONGLONG*)&ft2) - *((ULONGLONG*)&ft1))/10000000.0;
}

// used to create a relative time from the systemtime when only systemtime is stored in Sysinternals DbgView files.
// the reverse (creating system-time from relative times) makes no sense.
void DBLogReader::GetRelativeTime(Line& line)
{
	if (line.time != 0.0)		// if relative time is already filled in do nothing
		return;

	if (m_linenumber == 1)
	{
		m_firstFiletime = line.systemTime;
		return;
	}

	line.time = GetDifference(m_firstFiletime, line.systemTime);
}

void DBLogReader::AddLine(const std::string& data)
{
	++m_linenumber;
	Line line;
	switch (m_fileType)
	{
	case FileType::Unknown:
	case FileType::AsciiText:
		return FileReader::AddLine(data);
	case FileType::Sysinternals:
		ReadSysInternalsLogFileMessage(data, line);
		GetRelativeTime(line);
		break;
	case FileType::DebugViewPP1:
	case FileType::DebugViewPP2:
		if (m_linenumber == 1)	// ignore the header line
		{
			return;
		}
		ReadLogFileMessage(data, line);
		break;
	default:
		assert(false);
	}

	Add(line.time, line.systemTime, line.pid, line.processName.c_str(), line.message.c_str());
}

} // namespace debugviewpp 
} // namespace fusion
