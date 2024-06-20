// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <sstream>
#include <iomanip>
#include "Win32/Utilities.h"
#include "Win32/Win32Lib.h"
#include "CobaltFusion/stringbuilder.h"
#include "DebugViewppLib/Conversions.h"

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
    return GetDateText(Win32::FileTimeToSystemTime(Win32::FileTimeToLocalFileTime(ft)));
}

std::string GetTimeText(const SYSTEMTIME& st)
{
    char buf[32];
    sprintf_s(buf, "%02d:%02d:%02d.%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    return buf;
}

std::string GetDateTimeText(const FILETIME& filetime)
{
    // convert the FILETIME from UTC to local timezone so the time so written as 'clocktime'
    auto st = Win32::FileTimeToSystemTime(Win32::FileTimeToLocalFileTime(filetime));
    char buf[64];
    sprintf_s(buf, "%04d/%02d/%02d %02d:%02d:%02d.%03d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    return buf;
}

std::string GetTimeText(const FILETIME& ft)
{
    if (ft.dwHighDateTime == 0 && ft.dwLowDateTime == 0)
    {
        return "0"; // prevent endlessly repeating exception messageboxes when reading a corrupted file
    }
    return GetTimeText(Win32::FileTimeToSystemTime(Win32::FileTimeToLocalFileTime(ft)));
}

SYSTEMTIME GetSystemTime(WORD year, WORD month, WORD day)
{
    SYSTEMTIME st = {};
    st.wYear = year;
    st.wMonth = month;
    st.wDay = day;
    return st;
}

USTimeConverter::USTimeConverter() :
    m_lastFileTime(),
    m_firstValue(true)
{
}

FILETIME USTimeConverter::USTimeToFiletime(WORD h, WORD m, WORD s, WORD /*ms*/)
{
    if (m_firstValue)
    {
        m_firstValue = false;
        auto st = GetSystemTime(2000, 1, 1);
        st.wHour = h;
        st.wMinute = m;
        st.wSecond = s;
        st.wMilliseconds = 0; // todo: why is 'ms' not used?
        m_lastFileTime = Win32::SystemTimeToFileTime(st);
    }

    auto st = Win32::FileTimeToSystemTime(m_lastFileTime);
    st.wHour = h;
    st.wMinute = m;
    st.wSecond = s;
    st.wMilliseconds = 0;
    auto ft = Win32::SystemTimeToFileTime(st);

    if (ft < m_lastFileTime) // is ft before m_lastFileTime, then assume it is on the next day.
    {
        st.wDay += 1;
    }
    m_lastFileTime = Win32::SystemTimeToFileTime(st);
    return Win32::LocalFileTimeToFileTime(ft); // convert to UTC
}

// read localtime in format "hh:mm:ss tt" (AM/PM postfix)
bool USTimeConverter::ReadLocalTimeUSRegion(const std::string& text, FILETIME& ft)
{
    std::istringstream is(text);
    WORD h = 0;
    WORD m = 0;
    WORD s = 0;
    char c1 = 0;
    char c2 = 0;
    char p1 = 0;
    char p2 = 0;
    is >> h >> c1 >> m >> c2 >> s;

    if (!(is && c1 == ':' && c2 == ':'))
    {
        return false;
    }
    if (h == 12)
    {
        h = 0;
    }

    is >> p1 >> p2;
    if (is && p1 == 'P' && p2 == 'M')
    {
        h += 12;
    }

    ft = USTimeToFiletime(h, m, s, 0);
    return true;
}

// read localtime in format "hh:mm:ss.ms tt" (AM/PM postfix)
bool USTimeConverter::ReadLocalTimeUSRegionMs(const std::string& text, FILETIME& ft)
{
    std::istringstream is(text);

    WORD h = 0;
    WORD m = 0;
    WORD s = 0;
    WORD ms = 0;
    char c1 = 0;
    char c2 = 0;
    char p1 = 0;
    char p2 = 0;
    char d1 = 0;

    is >> h >> c1 >> m >> c2 >> s >> d1 >> ms;
    if (!(is && c1 == ':' && c2 == ':' && d1 == '.'))
    {
        return false;
    }
    if (h == 12)
    {
        h = 0;
    }

    is >> p1 >> p2;
    if (is && p1 == 'P' && p2 == 'M')
    {
        h += 12;
    }

    ft = USTimeToFiletime(h, m, s, ms);
    return true;
}

} // namespace debugviewpp
} // namespace fusion
