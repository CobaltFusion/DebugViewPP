// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <fstream>
#include <algorithm>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "Win32Lib/Win32Lib.h"
#include "Win32Lib/Utilities.h"
#include "DebugView++Lib/LogFile.h"
#include "DebugView++Lib/FileIO.h"
#include "DebugView++Lib/Conversions.h"

namespace fusion {
namespace debugviewpp {

class TabSplitter
{
public:
	explicit TabSplitter(const std::string& text) :
		m_it(text.begin()),
		m_end(text.end())
	{
	}

	std::string GetNext()
	{
		auto it = std::find(m_it, m_end, '\t');
		std::string s(m_it, it);
		m_it = it == m_end ? it : it + 1;
		return s;
	}

	std::string GetTail() const
	{
		return std::string(m_it, m_end);
	}

private:
	std::string::const_iterator m_it;
	std::string::const_iterator m_end;
};

FILETIME MakeFileTime(uint64_t t)
{
	uint32_t mask = ~0U;
	FILETIME ft;
	ft.dwHighDateTime = (t >> 32) & mask;
	ft.dwLowDateTime = t & mask;
	return ft;
}

FILETIME MakeFileTime(const std::string text)
{
	SYSTEMTIME st = { 0 };
	std::istringstream is(text);
	char c1, c2, c3, c4, c5, c6;
	if (!((is >> st.wYear >> c1 >> st.wMonth >> c2 >> st.wDay >> std::noskipws >> c3 >> st.wHour >> c4 >> st.wMinute >> c5 >> st.wSecond >> c6 >> st.wMilliseconds) 
		&& c1 == '/' && c2 == '/' && c3 == ' ' && c4 == ':' && c5 == ':' && c6 == '.'))
	{
		st = Win32::FileTimeToSystemTime(FILETIME());
	}

	return Win32::LocalFileTimeToFileTime(Win32::SystemTimeToFileTime(st));
}

bool ReadTime(const std::string& s, double& time)
{
	std::istringstream is(s);
	return is >> time && is.eof();
}

bool FileExists(const char *filename)
{
	std::ifstream ifile(filename);
	return ifile.is_open();
}

std::string FileTypeToString(FileType::type value)
{
	switch (value)
	{
	case FileType::DebugViewPP1: return "DebugView++ Logfile v1";
	case FileType::DebugViewPP2: return "DebugView++ Logfile v2";
	case FileType::Sysinternals: return "Sysinternals Debugview Logfile";
	case FileType::AsciiText: return "ASCII text file";
	case FileType::UTF8: return "Unicode UTF-8";
	case FileType::UTF16BE: return "Unicode UTF16BE";
	case FileType::UTF16LE: return "Unicode UTF16LE";
	default: break;
	}
	return "Unimplemented file type";
}

FileType::type IdentifyFile(const std::wstring& filename)
{
	{
		// Encoding detection is very complex, see #107
		std::ifstream fs(filename, std::ios::binary);
		std::vector<unsigned char> buffer(3);
		fs.read((char*)buffer.data(), buffer.size());
		if (buffer[0] == 0xfe && buffer[1] == 0xff)
		{
			return FileType::UTF16BE;
		}
		if (buffer[0] == 0xff && buffer[1] == 0xfe)
		{
			return FileType::UTF16LE;
		}
		if (buffer[0] == 0xef && buffer[1] == 0xbb && buffer[2] == 0xbf)
		{
			return FileType::UTF8;
		}
	}

	std::ifstream is(filename);
	std::string line;
	if (!std::getline(is, line))
		return FileType::Unknown;

	// first we check for our own header
	auto trimmed = boost::trim_copy_if(line, boost::is_any_of(" \r\n\t"));
	if (boost::ends_with(trimmed, g_debugViewPPIdentification1))
	{
		return FileType::DebugViewPP1;
	}
	if (boost::ends_with(trimmed, g_debugViewPPIdentification2))
	{
		return FileType::DebugViewPP2;
	}

	// if the extention is .txt (and we did not find our own header)
	// we say it is an ascii-file.
	if (boost::iends_with(filename, ".txt"))
		return FileType::AsciiText;

	// .log files are potentially sysinternals dbgview files
	if (boost::iends_with(filename, ".log"))
	{
		// to test for sysinternals debugview-logfiles, we need to check the second line 
		// since the first line contains the computer-name in some cases (depending on how it was saved)
		// logfiles with only one line are not that interesting anyway.
		if (!std::getline(is, line))
			return FileType::AsciiText;

		// if the second line contains 2 or 3 tabs characters, we say it's a sysinternals debugview-logfile 
		// two kinds of lines are logged, kernel message lines and process message lines, the latter have an extra PID Columnn
		auto tabs = std::count(line.begin(), line.end(), '\t');
		if (tabs == 2 || tabs == 3)
			return FileType::Sysinternals;
	}
	return FileType::AsciiText;
}

bool IsBinaryFileType(FileType::type filetype)
{
	switch (filetype)
	{
	case FileType::UTF16BE:
	case FileType::UTF16LE:
	case FileType::UTF8:
		return true;
	default:
		return false;
	}
}

// read localtime in format "HH:mm:ss.ms"
bool ReadLocalTimeMs(const std::string& text, FILETIME& ft)
{
	std::istringstream is(text);
	WORD h, m, s, ms;
	char c1, c2, d1;
	if (!((is >> h >> c1 >> m >> c2 >> s >> d1 >> ms) && c1 == ':' && c2 == ':' && d1 == '.'))
		return false;

	SYSTEMTIME st = Win32::FileTimeToSystemTime(FILETIME());
	st.wHour = h;
	st.wMinute = m;
	st.wSecond = s;
	st.wMilliseconds = ms;
	ft = Win32::LocalFileTimeToFileTime(Win32::SystemTimeToFileTime(st));
	return true;
}

// read localtime in format "HH:mm:ss"
bool ReadLocalTime(const std::string& text, FILETIME& ft)
{
	std::istringstream is(text);
	WORD h, m, s;
	char c1, c2;
	if (!((is >> h >> c1 >> m >> c2 >> s) && c1 == ':' && c2 == ':'))
		return false;

	SYSTEMTIME st = Win32::FileTimeToSystemTime(FILETIME());
	st.wHour = h;
	st.wMinute = m;
	st.wSecond = s;
	st.wMilliseconds = 0;
	ft = Win32::LocalFileTimeToFileTime(Win32::SystemTimeToFileTime(st));
	return true;
}

std::istream& ReadLogFileMessage(std::istream& is, Line& line)
{
	std::string data;
	if (!std::getline(is, data))
		return is;
	if (!ReadLogFileMessage(data, line))
		is.setstate(std::ios_base::failbit);
	return is;
}

bool ReadSysInternalsLogFileMessage(const std::string& data, Line& line, USTimeConverter& converter)
{
	TabSplitter split(data);
	auto col1 = split.GetNext();
	auto col2 = split.GetNext();
	auto col3 = split.GetTail();

	// depending on regional settings Sysinternals debugview logs time differently.
	// we support the four most common formats
	// 'hh:MM:SS.mmm tt', 'hh:MM:SS tt', 'HH:MM:SS.mmm' and 'HH:MM:SS' 
	if (!converter.ReadLocalTimeUSRegionMs(col2,line.systemTime))			// try hh:MM:SS.mmm tt
		if (!converter.ReadLocalTimeUSRegion(col2,line.systemTime))		// try hh:MM:SS tt
			if (!ReadLocalTimeMs(col2, line.systemTime))		// try HH:MM:SS.mmm
				if (!ReadLocalTime(col2, line.systemTime))      // try HH:MM:SS
					ReadTime(col2, line.time);					// otherwise assume relative time: S.mmmmmm

	if (!col3.empty() && col3[0] == '[')						// messages from processes are preceeded by [pid], but kernel messages do not have a prefix
	{
		line.processName = "[unavailable]";
		std::istringstream is3(col3);
		char c1, c2, c3;
		if (is3 >> std::noskipws >> c1 >> line.pid >> c2 >> c3 && c1 == '[' && c2 == ']' && c3 == ' ' && std::getline(is3, line.message))
			return true;
	}
	else
	{
		line.processName = "[kernel]";
	}
	line.message = split.GetTail();
	return true;
}

bool ReadLogFileMessage(const std::string& data, Line& line)
{
	try
	{
		TabSplitter split(data);
		line.time = boost::lexical_cast<double>(split.GetNext());
		line.systemTime = MakeFileTime(split.GetNext());
		line.pid = boost::lexical_cast<DWORD>(split.GetNext());
		line.processName = split.GetNext();
		line.message = split.GetTail();
	}
	catch (std::exception& ex)
	{
		line.message = stringbuilder() << "Exception: '" << ex.what() << "' occurred processing line: " << data;
	}
	return true;
}

std::ostream& operator<<(std::ostream& os, const FILETIME& ft)
{
	uint64_t hi = ft.dwHighDateTime;
	uint64_t lo = ft.dwLowDateTime;
	return os << ((hi << 32) | lo);
}

void OpenLogFile(std::ofstream& ofstream, const std::wstring& filename, OpenMode::type mode)
{
	ofstream.open(filename, mode == OpenMode::Truncate ? std::ofstream::trunc : std::ofstream::app);
	
	if (mode == OpenMode::Truncate)
	{
		// intentionally maintain the same amount of Columnns, so it is always easy to parse by csv import tools
		WriteLogFileMessage(ofstream, 0, FILETIME(), 0, "DebugView++.exe", g_debugViewPPIdentification1);
	}
}

std::string GetOffsetText(double time)
{
	char buf[32];
	sprintf_s(buf, "%.06f", time);
	return buf;
}

void WriteLogFileMessage(std::ofstream& ofstream, double time, FILETIME filetime, DWORD pid, const std::string& processName, std::string message)
{
	boost::trim_right_if(message, boost::is_any_of(" \r\n\t")); 
	ofstream <<
		GetOffsetText(time) << "\t" <<
		GetDateTimeText(filetime) << "\t"<<
		pid << "\t" <<
		processName << "\t" <<
		message << "\n";
}

} // namespace debugviewpp 
} // namespace fusion
