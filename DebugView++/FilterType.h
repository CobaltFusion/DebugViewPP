// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include <regex>
#include "MatchType.h"

namespace fusion {
namespace debugviewpp {
 
struct FilterType
{
	enum type
	{
		Include,
		Exclude,
		Highlight,
		Token,
		Stop,
		Track,
		Once
	};
};

struct Filter
{
	Filter();
	Filter(const std::string& text, MatchType::type matchType, FilterType::type filterType, COLORREF bgColor = RGB(255, 255, 255), COLORREF fgColor = RGB(0, 0, 0), bool enable = true, int matchCount = 0);

	std::string text;
	std::regex re;
	MatchType::type matchType;
	FilterType::type filterType;
	COLORREF bgColor;
	COLORREF fgColor;
	bool enable;
	int matchCount;
};

int FilterTypeToInt(FilterType::type value);

FilterType::type IntToFilterType(int value);

std::string FilterTypeToString(FilterType::type value);

const wchar_t* EnumToWCharPtr(FilterType::type value);

FilterType::type StringToFilterType(const std::string& s);

} // namespace debugviewpp 
} // namespace fusion
