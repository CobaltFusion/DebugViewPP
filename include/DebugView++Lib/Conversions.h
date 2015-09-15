// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <windows.h>
#include <string>
#include "Win32Lib/Win32Lib.h"

namespace fusion {
namespace debugviewpp {

std::string GetTimeText(double time);
std::string GetDateText(const SYSTEMTIME& st);
std::string GetDateText(const FILETIME& ft);
std::string GetDateTimeText(const FILETIME& filetime);
std::string GetTimeText(const SYSTEMTIME& st);
std::string GetTimeText(const FILETIME& ft);

template <typename CharT>
std::basic_string<CharT> TabsToSpaces(const std::basic_string<CharT>& s, int tabsize = 4)
{
	std::basic_string<CharT> result;
	result.reserve(s.size() + 3*tabsize);
	for (auto it = s.begin(); it != s.end(); ++it)
	{
		if (*it == CharT('\t'))
		{
			do
			{
				result.push_back(CharT(' '));
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

class USTimeConverter
{
public:
	USTimeConverter();

	bool USTimeConverter::ReadLocalTimeUSRegion(const std::string& text, FILETIME& ft);
	bool USTimeConverter::ReadLocalTimeUSRegionMs(const std::string& text, FILETIME& ft);

private:
	FILETIME USTimeToFiletime(WORD h, WORD m, WORD s, WORD ms);
	FILETIME m_lastFileTime;
	bool m_firstValue;
};

} // namespace debugviewpp 
} // namespace fusion
