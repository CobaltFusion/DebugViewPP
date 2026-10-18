// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <stdexcept>
#include "DebugViewppLib/Colors.h"
#include "DebugViewppLib/FilterJson.h"
#include "DebugViewppLib/LogFilter.h"

namespace fusion {
namespace debugviewpp {

using nlohmann::json;

namespace {

constexpr int FilterFileVersion = 2;

json ColorToJson(COLORREF color)
{
    if (color == Colors::Auto)
    {
        return {{"auto", true}};
    }
    return {{"r", GetRValue(color)}, {"g", GetGValue(color)}, {"b", GetBValue(color)}};
}

int ColorComponent(const json& json, const char* name)
{
    auto it = json.find(name);
    if (it == json.end() || !it->is_number_integer())
    {
        throw std::runtime_error(std::string("color: '") + name + "' is missing or not a number");
    }
    const int value = it->get<int>();
    if (value < 0 || value > 255)
    {
        throw std::runtime_error(std::string("color: '") + name + "' must be between 0 and 255");
    }
    return value;
}

COLORREF ColorFromJson(const json& json)
{
    if (!json.is_object())
    {
        throw std::runtime_error("color: an object is expected");
    }
    auto automatic = json.find("auto");
    if (automatic != json.end() && automatic->is_boolean() && automatic->get<bool>())
    {
        return Colors::Auto;
    }
    return RGB(ColorComponent(json, "r"), ColorComponent(json, "g"), ColorComponent(json, "b"));
}

std::string RequiredString(const json& json, const char* name)
{
    auto it = json.find(name);
    if (it == json.end() || !it->is_string())
    {
        throw std::runtime_error(std::string("filter: '") + name + "' is missing or not a string");
    }
    return it->get<std::string>();
}

COLORREF OptionalColor(const json& json, const char* name, COLORREF defaultColor)
{
    auto it = json.find(name);
    return it == json.end() ? defaultColor : ColorFromJson(*it);
}

} // namespace

json ToJson(const Filter& filter)
{
    return {
        {"enable", filter.enable},
        {"text", filter.text},
        {"matchType", MatchTypeToString(filter.matchType)},
        {"filterType", FilterTypeToString(filter.filterType)},
        {"backColor", ColorToJson(filter.bgColor)},
        {"textColor", ColorToJson(filter.fgColor)},
    };
}

json ToJson(const std::vector<Filter>& filters)
{
    json array = json::array();
    for (const auto& filter : filters)
    {
        array.push_back(ToJson(filter));
    }
    return array;
}

Filter FilterFromJson(const json& json)
{
    if (!json.is_object())
    {
        throw std::runtime_error("filter: an object is expected");
    }

    bool enable = true;
    auto enableIt = json.find("enable");
    if (enableIt != json.end())
    {
        if (!enableIt->is_boolean())
        {
            throw std::runtime_error("filter: 'enable' must be true or false");
        }
        enable = enableIt->get<bool>();
    }

    return MakeFilter(
        RequiredString(json, "text"),
        StringToMatchType(RequiredString(json, "matchType")),
        StringToFilterType(RequiredString(json, "filterType")),
        OptionalColor(json, "backColor", RGB(255, 255, 255)),
        OptionalColor(json, "textColor", RGB(0, 0, 0)),
        enable);
}

std::vector<Filter> FiltersFromJson(const json& json)
{
    if (!json.is_array())
    {
        throw std::runtime_error("filters: an array is expected");
    }

    std::vector<Filter> filters;
    for (const auto& item : json)
    {
        filters.push_back(FilterFromJson(item));
    }
    return filters;
}

std::string FilterDataToJson(const FilterData& data)
{
    json root = {
        {"version", FilterFileVersion},
        {"name", data.name},
        {"messageFilters", ToJson(data.filter.messageFilters)},
        {"processFilters", ToJson(data.filter.processFilters)},
    };
    return root.dump(4);
}

FilterData FilterDataFromJson(const std::string& text)
{
    json root = json::parse(text, nullptr, false);
    if (root.is_discarded() || !root.is_object())
    {
        throw std::runtime_error("the filter file is not valid JSON");
    }

    auto version = root.find("version");
    if (version != root.end() && version->is_number_integer() && version->get<int>() > FilterFileVersion)
    {
        throw std::runtime_error("the filter file was written by a newer version of DebugView++");
    }

    FilterData data;
    data.name = RequiredString(root, "name");
    auto messageFilters = root.find("messageFilters");
    auto processFilters = root.find("processFilters");
    if (messageFilters != root.end())
    {
        data.filter.messageFilters = FiltersFromJson(*messageFilters);
    }
    if (processFilters != root.end())
    {
        data.filter.processFilters = FiltersFromJson(*processFilters);
    }
    return data;
}

} // namespace debugviewpp
} // namespace fusion
