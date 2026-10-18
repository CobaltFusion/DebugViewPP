// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include "CobaltFusion/StringUtil.h"
#include "Win32/Registry.h"
#include "CobaltFusion/stringbuilder.h"
#include "DebugViewppLib/Colors.h"
#include "DebugViewppLib/Filter.h"

namespace fusion {
namespace debugviewpp {

std::string MatchKey(const RegexMatch& match, MatchType::type matchType)
{
    if (matchType == MatchType::RegexGroups && match.size() > 1)
    {
        std::string key;
        for (size_t i = 1; i < match.size(); ++i)
        {
            if (!key.empty())
            {
                key.append(1, '\0');
            }
            key += ToLower(std::string(match.Group(i)));
        }
        return key;
    }
    return ToLower(std::string(match.Group()));
}

Filter::Filter() :
    matchType(MatchType::Simple),
    filterType(FilterType::Include),
    bgColor(RGB(255, 255, 255)),
    fgColor(RGB(0, 0, 0)),
    enable(true),
    matched(false)
{
}

Filter::Filter(const std::string& text, MatchType::type matchType, FilterType::type filterType, COLORREF bgColor, COLORREF fgColor, bool enable, bool matched) :
    text(text),
    re(MakePattern(matchType, text), matchType != MatchType::RegexCase),
    matchType(matchType),
    filterType(filterType),
    bgColor(bgColor),
    fgColor(fgColor),
    enable(enable),
    matched(matched)
{
}

void SaveFilterSettings(const std::vector<Filter>& filters, CRegKey& reg)
{
    int i = 0;
    for (auto& filter : filters)
    {
        CRegKey regFilter;
        regFilter.Create(reg, WStr(wstringbuilder() << L"Filter" << i));
        regFilter.SetStringValue(L"", WStr(filter.text));
        regFilter.SetDWORDValue(L"MatchType", MatchTypeToInt(filter.matchType));
        regFilter.SetDWORDValue(L"FilterType", FilterTypeToInt(filter.filterType));
        regFilter.SetDWORDValue(L"Type", FilterTypeToInt(filter.filterType));
        regFilter.SetDWORDValue(L"BgColor", filter.bgColor);
        regFilter.SetDWORDValue(L"FgColor", filter.fgColor);
        regFilter.SetDWORDValue(L"Enable", static_cast<DWORD>(filter.enable));
        ++i;
    }
}

// Temporary backward compatibilty for loading FilterType::MatchColor:
Filter MakeFilter(const std::string& text, MatchType::type matchType, FilterType::type filterType, COLORREF bgColor, COLORREF fgColor, bool enable, bool matched)
{
    if (filterType == FilterType::MatchColor)
    {
        filterType = FilterType::Token;
        bgColor = Colors::Auto;
    }
    return Filter(text, matchType, filterType, bgColor, fgColor, enable, matched);
}

void LoadFilterSettings(std::vector<Filter>& filters, CRegKey& reg)
{
    for (int i = 0;; ++i)
    {
        CRegKey regFilter;
        if (regFilter.Open(reg, WStr(wstringbuilder() << L"Filter" << i)) != ERROR_SUCCESS)
        {
            break;
        }

        filters.push_back(MakeFilter(
            Str(Win32::RegGetStringValue(regFilter)),
            IntToMatchType(Win32::RegGetDWORDValue(regFilter, L"MatchType", MatchType::Regex)),
            IntToFilterType(Win32::RegGetDWORDValue(regFilter, L"Type")),
            Win32::RegGetDWORDValue(regFilter, L"BgColor", Colors::BackGround),
            Win32::RegGetDWORDValue(regFilter, L"FgColor", Colors::Text),
            Win32::RegGetDWORDValue(regFilter, L"Enable", 1) != 0));
    }
}

bool SameDefinition(const Filter& a, const Filter& b)
{
    return a.text == b.text && a.matchType == b.matchType && a.filterType == b.filterType && a.bgColor == b.bgColor && a.fgColor == b.fgColor && a.enable == b.enable;
}

bool SameDefinition(const std::vector<Filter>& a, const std::vector<Filter>& b)
{
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](const Filter& x, const Filter& y) { return SameDefinition(x, y); });
}

bool SameDefinition(const LogFilter& a, const LogFilter& b)
{
    return SameDefinition(a.messageFilters, b.messageFilters) && SameDefinition(a.processFilters, b.processFilters);
}

bool IsIncluded(std::vector<Filter>& filters, const std::string& text, MatchColors& matchColors)
{
    for (auto& filter : filters)
    {
        if (!filter.enable)
        {
            continue;
        }

        if (filter.filterType == FilterType::Exclude && filter.re.Search(text))
        {
            return false;
        }
    }

    bool included = false;
    bool includeFilterPresent = false;
    for (auto& filter : filters)
    {
        if (!filter.enable)
        {
            continue;
        }

        if (filter.bgColor == Colors::Auto)
        {
            filter.re.ForEachMatch(text, [&](const RegexMatch& match) {
                auto key = MatchKey(match, filter.matchType);
                if (matchColors.find(key) == matchColors.end())
                {
                    matchColors.emplace(std::make_pair(key, GetRandomBackColor()));
                }
            });
        }

        if (filter.filterType == FilterType::Include)
        {
            includeFilterPresent = true;
            included |= filter.re.Search(text);
        }

        if (filter.filterType == FilterType::Once && filter.re.Search(text))
        {
            included |= !filter.matched;
            filter.matched = true;
        }
    }

    return !includeFilterPresent || included;
}

bool MatchFilterType(const std::vector<Filter>& filters, FilterType::type type, const std::string& text)
{
    for (auto& filter : filters)
    {
        if (filter.enable && filter.filterType == type && filter.re.Search(text))
        {
            return true;
        }
    }

    return false;
}

} // namespace debugviewpp
} // namespace fusion
