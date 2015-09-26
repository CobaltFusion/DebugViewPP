// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DebugView++Lib/Colors.h"
#include "Win32Lib/utilities.h"
#include "Win32Lib/Win32Lib.h"
#include "DebugView++Lib/Filter.h"

namespace fusion {
namespace debugviewpp {

ColorMatch::ColorMatch(std::string text, COLORREF color) :
	text(text), color(color)
{
}

Filter::Filter() :
	matchType(MatchType::Simple),
	filterType(FilterType::Include),
	bgColor(RGB(255, 255, 255)),
	fgColor(RGB(  0,   0,   0)),
	enable(true),
	matched(false)
{
}

Filter::Filter(const std::string& text, MatchType::type matchType, FilterType::type filterType, COLORREF bgColor, COLORREF fgColor, bool enable, bool matched) :
	text(text), re(MakePattern(matchType, text), std::regex_constants::icase | std::regex_constants::optimize), matchType(matchType), filterType(filterType), bgColor(bgColor), fgColor(fgColor), enable(enable), matched(matched)
{
}

void SaveFilterSettings(const std::vector<Filter>& filters, CRegKey& reg)
{
	int i = 0;
	for (auto it = filters.begin(); it != filters.end(); ++it, ++i)
	{
		CRegKey regFilter;
		regFilter.Create(reg, WStr(wstringbuilder() << L"Filter" << i));
		regFilter.SetStringValue(L"", WStr(it->text.c_str()));
		regFilter.SetDWORDValue(L"MatchType", MatchTypeToInt(it->matchType));
		regFilter.SetDWORDValue(L"FilterType", FilterTypeToInt(it->filterType));
		regFilter.SetDWORDValue(L"Type", FilterTypeToInt(it->filterType));
		regFilter.SetDWORDValue(L"BgColor", it->bgColor);
		regFilter.SetDWORDValue(L"FgColor", it->fgColor);
		regFilter.SetDWORDValue(L"Enable", it->enable);
	}
}

void LoadFilterSettings(std::vector<Filter>& filters, CRegKey& reg)
{
	for (int i = 0; ; ++i)
	{
		CRegKey regFilter;
		if (regFilter.Open(reg, WStr(wstringbuilder() << L"Filter" << i)) != ERROR_SUCCESS)
			break;

		filters.push_back(Filter(
			Str(RegGetStringValue(regFilter)),
			IntToMatchType(RegGetDWORDValue(regFilter, L"MatchType", MatchType::Regex)),
			IntToFilterType(RegGetDWORDValue(regFilter, L"Type")),
			RegGetDWORDValue(regFilter, L"BgColor", Colors::BackGround),
			RegGetDWORDValue(regFilter, L"FgColor", Colors::Text),
			RegGetDWORDValue(regFilter, L"Enable", 1) != 0));
	}
}

bool IsIncluded(std::vector<Filter>& filters, const std::string& text, ColorMatches& colorMatches)
{
	for (auto it = filters.begin(); it != filters.end(); ++it)
	{
		if (!it->enable)
			continue;

		if (it->filterType == FilterType::Exclude && std::regex_search(text, it->re))
			return false;
	}

	bool included = false;
	bool includeFilterPresent = false;
	for (auto it = filters.begin(); it != filters.end(); ++it)
	{
		if (!it->enable)
			continue;

		if (it->filterType == FilterType::MatchColor)
		{
			std::smatch match;
			if (std::regex_search(text, match, it->re))
				colorMatches.push_back(ColorMatch(match[0].str(), GetRandomBackColor()));
		}

		if (!includeFilterPresent && it->filterType == FilterType::Include)
		{
			includeFilterPresent = true;
			included |= std::regex_search(text, it->re);
		}

		if (it->filterType == FilterType::Once && std::regex_search(text, it->re))
		{
			included = it->matched;
			it->matched = true;
			return included;
		}
	}

	return !includeFilterPresent || included;
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
