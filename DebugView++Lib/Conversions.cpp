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

std::string GetTimeText(const FILETIME& ft)
{
	return GetTimeText(FileTimeToSystemTime(FileTimeToLocalFileTime(ft)));
}

USTimeConverter::USTimeConverter()
    : m_lastFileTime(FILETIME())
{
}

// read localtime in format "hh:mm:ss tt" (AM/PM postfix)
bool USTimeConverter::ReadLocalTimeUSRegion(const std::string& text, FILETIME& ft)
{
	std::istringstream is(text);
	WORD h, m, s;
	WORD day = 1;
	char c1, c2, p1, p2;
	if (!((is >> h >> c1 >> m >> c2 >> s) && c1 == ':' && c2 == ':'))
		return false;
	if (h == 12) 
		h = 0;

	if (is >> p1 >> p2 && p1 == 'P' && p2 == 'M')
		h += 12;

    SYSTEMTIME st = { 2000, 1, 0, 0 }; // wYear, wMonth, wDayOfWeek, wDay; months starts are 1
	st.wHour = h;
	st.wMinute = m;
	st.wSecond = s;
	st.wMilliseconds = 0;
	st.wDay = day;
	ft = SystemTimeToFileTime(st);

    //if (ft < m_lastFileTime)
    //{
    //    SYSTEMTIME lst = FileTimeToSystemTime(m_lastFileTime);
    //    lst.wDay += 1;
    //}

	LocalFileTimeToFileTime(&ft, &ft);		// convert to UTC
	return true;
}

// read localtime in format "hh:mm:ss.ms tt" (AM/PM postfix)
bool USTimeConverter::ReadLocalTimeUSRegionMs(const std::string& text, FILETIME& ft)
{
	std::istringstream is(text);

	WORD h, m, s, ms;
	WORD day = 1;
	char c1, c2, p1, p2, d1;
	if (!((is >> h >> c1 >> m >> c2 >> s >> d1 >> ms) && c1 == ':' && c2 == ':' && d1 == '.'))
		return false;
	if (is >> p1 >> p2 && p1 == 'P' && p2 == 'M')
	{
		if (h == 12) 
		{
			h = 0;
			day = 2;
		}
	}

	SYSTEMTIME st = {0};
	st.wYear = 2000;
	st.wMonth = 1;
	st.wHour = h;
	st.wMinute = m;
	st.wSecond = s;
	st.wMilliseconds = ms;
	st.wDay = day;
	ft = SystemTimeToFileTime(st);
	LocalFileTimeToFileTime(&ft, &ft);		// convert to UTC
	return true;
}


} // namespace debugviewpp 
} // namespace fusion
