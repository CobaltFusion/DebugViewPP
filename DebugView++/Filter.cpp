// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Filter.h"

namespace fusion {
namespace debugviewpp {

Filter::Filter() :
	matchType(MatchType::Simple),
	filterType(FilterType::Include),
	bgColor(RGB(255, 255, 255)),
	fgColor(RGB(  0,   0,   0)),
	enable(true)
{
}

Filter::Filter(const std::string& text, MatchType::type matchType, FilterType::type filterType, COLORREF bgColor, COLORREF fgColor, bool enable, int matchCount) :
	text(text), re(MakePattern(matchType, text), std::regex_constants::icase | std::regex_constants::optimize), matchType(matchType), filterType(filterType), bgColor(bgColor), fgColor(fgColor), enable(enable), matchCount(matchCount)
{
}

bool IsIncluded(std::vector<Filter>& filters, const std::string& text)
{
	bool included = false;
	bool includeFilterPresent = false;
	for (auto it = filters.begin(); it != filters.end(); ++it)
	{
		if (it->enable && it->filterType == FilterType::Include)
		{
			includeFilterPresent = true;
			included |= std::regex_search(text, it->re);
		}
	}

	if (!includeFilterPresent) 
		included = true;

	for (auto it = filters.begin(); it != filters.end(); ++it)
	{
		if (it->enable && it->filterType == FilterType::Exclude && std::regex_search(text, it->re))
			return false;
	}

	for (auto it = filters.begin(); it != filters.end(); ++it)
	{
		if (it->enable && it->filterType == FilterType::Once && std::regex_search(text, it->re))
			return ++it->matchCount == 1;
	}

	return included;
}

bool MatchFilterType(const std::vector<Filter>& filters, FilterType::type type, const std::string& text)
{
	for (auto it = filters.begin(); it != filters.end(); ++it)
	{
		if (it->enable && it->filterType == type && std::regex_search(text, it->re))
			return true;
	}

	return false;
}

} // namespace debugviewpp 
} // namespace fusion
