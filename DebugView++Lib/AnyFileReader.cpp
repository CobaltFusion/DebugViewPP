// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <cassert>
#include "DebugView++Lib/FileIO.h"
#include "DebugView++Lib/AnyFileReader.h"
#include "DebugView++Lib/LineBuffer.h"
#include "DebugView++Lib/Line.h"

namespace fusion {
namespace debugviewpp {

AnyFileReader::AnyFileReader(Timer& timer, ILineBuffer& linebuffer, FileType::type filetype, const std::wstring& filename, bool keeptailing) :
	FileReader(timer, linebuffer, filetype, filename, keeptailing),
	m_linenumber(0)
{
}

uint64_t FileTimeToUInt64(const FILETIME& ft)
{
	ULARGE_INTEGER value;
	value.LowPart = ft.dwLowDateTime;
	value.HighPart = ft.dwHighDateTime;
	return value.QuadPart;
}

double GetDifference(FILETIME ft1, FILETIME ft2)
{
	return (FileTimeToUInt64(ft2) - FileTimeToUInt64(ft1)) * 100e-9;
}

// used to create a relative time from the systemtime when only systemtime is stored in Sysinternals DbgView files.
// the reverse (creating system-time from relative times) makes no sense.
void AnyFileReader::GetRelativeTime(Line& line)
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

void AnyFileReader::AddLine(const std::string& data)
{
	++m_linenumber;
	Line line;
	switch (m_fileType)
	{
	case FileType::Unknown:
	case FileType::AsciiText:
		return FileReader::AddLine(data);
	case FileType::Sysinternals:
		ReadSysInternalsLogFileMessage(data, line, m_converter);
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
		assert(false && "Unknown Filetype in AnyFileReader");
	}

	Add(line.time, line.systemTime, line.pid, line.processName, line.message);
}

void AnyFileReader::PreProcess(Line& line) const
{
	// intentionally empty (will negate the default behavior)
}

} // namespace debugviewpp 
} // namespace fusion
