// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "MatchType.h"
#include "FilterType.h"

#include "atlbase.h"

#include <string>
#include <regex>
#include <vector>
#include <unordered_map>

#pragma comment(lib, "DebugView++Lib.lib")

namespace fusion {
namespace debugviewpp {

using MatchColors = std::unordered_map<std::string, COLORREF>;

struct Filter
{
    Filter();
    Filter(const std::string& text, MatchType::type matchType, FilterType::type filterType, COLORREF bgColor = RGB(255, 255, 255), COLORREF fgColor = RGB(0, 0, 0), bool enable = true, bool matched = false);

    std::string text;
    std::regex re;
    MatchType::type matchType;
    FilterType::type filterType;
    COLORREF bgColor;
    COLORREF fgColor;
    bool enable;
    bool matched;
};

struct LogFilter
{
    std::vector<Filter> messageFilters;
    std::vector<Filter> processFilters;
};

void SaveFilterSettings(const std::vector<Filter>& filters, CRegKey& reg);
void LoadFilterSettings(std::vector<Filter>& filters, CRegKey& reg);

bool IsIncluded(std::vector<Filter>& filters, const std::string& text, MatchColors& matchColors);
bool MatchFilterType(const std::vector<Filter>& filters, FilterType::type type, const std::string& text);

std::string MatchKey(const std::smatch& match, MatchType::type matchType);

// Temporary backward compatibilty for loading FilterType::MatchColor:
Filter MakeFilter(const std::string& text, MatchType::type matchType, FilterType::type filterType, COLORREF bgColor = RGB(255, 255, 255), COLORREF fgColor = RGB(0, 0, 0), bool enable = true, bool matched = false);

} // namespace debugviewpp
} // namespace fusion
