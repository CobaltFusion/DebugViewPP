// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include <regex>
#include <vector>
#include "MatchType.h"
#include "FilterType.h"

#include "atlbase.h"

namespace fusion {
namespace debugviewpp {

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

struct LogFilter
{
	std::vector<Filter> messageFilters;
	std::vector<Filter> processFilters;
};

void SaveFilterSettings(const std::vector<Filter>& filters, CRegKey& reg);
void LoadFilterSettings(std::vector<Filter>& filters, CRegKey& reg);

bool IsIncluded(std::vector<Filter>& filters, const std::string& message);
bool MatchFilterType(const std::vector<Filter>& filters, FilterType::type type, const std::string& text);

} // namespace debugviewpp 
} // namespace fusion
