// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Win32Lib/utilities.h"
#include "Win32Lib/Win32Lib.h"

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

std::wstring TabsToSpaces(const std::wstring& s, int tabsize)
{
	std::wstring result;
	result.reserve(s.size() + 3*tabsize);
	for (auto it = s.begin(); it != s.end(); ++it)
	{
		if (*it == L'\t')
		{
			do
			{
				result.push_back(L' ');
			}
			while (result.size() % tabsize != 0);
		}
		else
		{
			result.push_back(*it);
		}
	}
	return result;
}

std::string TabsToSpaces(const std::string& s, int tabsize)
{
	std::string result;
	result.reserve(s.size() + 3*tabsize);
	for (auto it = s.begin(); it != s.end(); ++it)
	{
		if (*it == '\t')
		{
			do
			{
				result.push_back(' ');
			}
			while (result.size() % tabsize != 0);
		}
		else
		{
			result.push_back(*it);
		}
	}
	return result;
}

} // namespace debugviewpp 
} // namespace fusion
