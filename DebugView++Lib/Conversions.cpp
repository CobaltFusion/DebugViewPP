// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DebugView++Lib/Conversions.h"
#include "Win32Lib/utilities.h"
#include "Win32Lib/Win32Lib.h"
#include <sstream>

namespace fusion {
namespace debugviewpp {

std::string GetTimeText(double time)
{
	return stringbuilder() << std::fixed << std::setprecision(6) << time;
}

std::string GetDateText(const SYSTEMTIME& st)
{
	int size = GetDateFormatA(LOCALE_USER_DEFAULT, 0, &st, nullptr, nullptr, 0);
	std::vector<char> buf(size);
	GetDateFormatA(LOCALE_USER_DEFAULT, 0, &st, nullptr, buf.data(), size);
	return std::string(buf.data(), size - 1);
}

std::string GetDateText(const FILETIME& ft)
{
	return GetDateText(FileTimeToSystemTime(FileTimeToLocalFileTime(ft)));
}

std::string GetTimeText(const SYSTEMTIME& st)
{
	char buf[32];
	sprintf_s(buf, "%d:%02d:%02d.%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	return buf;
}

std::string GetDateTimeText(const FILETIME& filetime)
{
	// convert the FILETIME from UTC to local timezone so the time so written as 'clocktime'
	auto st = FileTimeToSystemTime(FileTimeToLocalFileTime(filetime));
	char buf[64];
	sprintf_s(buf, "%04d/%02d/%02d %02d:%02d:%02d.%03d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	return buf;
}

std::string GetTimeText(const FILETIME& ft)
{
	return GetTimeText(FileTimeToSystemTime(FileTimeToLocalFileTime(ft)));
}

SYSTEMTIME GetSystemTime(WORD year, WORD month, WORD day)
{
	SYSTEMTIME st = { year, month, 0, day }; // wYear, wMonth, wDayOfWeek, wDay
	return st;
}

USTimeConverter::USTimeConverter()
	: m_lastFileTime(FILETIME())
	, m_firstValue(true)
{
}

FILETIME USTimeConverter::USTimeToFiletime(WORD h, WORD m, WORD s, WORD ms)
{
	if (m_firstValue)
	{
		m_firstValue = false;
		auto st = GetSystemTime(2000, 1, 1);
		st.wHour = h;
		st.wMinute = m;
		st.wSecond = s;
		st.wMilliseconds = 0;
		m_lastFileTime = SystemTimeToFileTime(st);
	}

	auto st = FileTimeToSystemTime(m_lastFileTime);
	st.wHour = h;
	st.wMinute = m;
	st.wSecond = s;
	st.wMilliseconds = 0;
	auto ft = SystemTimeToFileTime(st);

	if (CompareFileTime(&ft, &m_lastFileTime) < 0)	// is ft before m_lastFileTime, then assume it is on the next day.
	{
		st.wDay += 1;
	}
	ft = SystemTimeToFileTime(st);
	m_lastFileTime = ft;
	LocalFileTimeToFileTime(&ft, &ft);		// convert to UTC
	return ft;
}

// read localtime in format "hh:mm:ss tt" (AM/PM postfix)
bool USTimeConverter::ReadLocalTimeUSRegion(const std::string& text, FILETIME& ft)
{
	std::istringstream is(text);
	WORD h, m, s;
	char c1, c2, p1, p2;
	if (!((is >> h >> c1 >> m >> c2 >> s) && c1 == ':' && c2 == ':'))
		return false;
	if (h == 12) 
		h = 0;

	if (is >> p1 >> p2 && p1 == 'P' && p2 == 'M')
		h += 12;

	ft = USTimeToFiletime(h, m, s, 0);
	return true;
}

// read localtime in format "hh:mm:ss.ms tt" (AM/PM postfix)
bool USTimeConverter::ReadLocalTimeUSRegionMs(const std::string& text, FILETIME& ft)
{
	std::istringstream is(text);

	WORD h, m, s, ms;
	char c1, c2, p1, p2, d1;
	if (!((is >> h >> c1 >> m >> c2 >> s >> d1 >> ms) && c1 == ':' && c2 == ':' && d1 == '.'))
		return false;
	if (h == 12) 
		h = 0;

	if (is >> p1 >> p2 && p1 == 'P' && p2 == 'M')
		h += 12;

	ft = USTimeToFiletime(h, m, s, ms);
	return true;
}


} // namespace debugviewpp 
} // namespace fusion
