// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// #include "stdafx.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include "DebugViewppLib/Colors.h"
#include "DebugViewppLib/ConfigFile.h"
#include "DebugViewppLib/LegacyConfig.h"
#include "DebugViewppLib/LogFilter.h"

namespace fusion {
namespace debugviewpp {

using boost::property_tree::ptree;

ptree MakePTree(COLORREF color)
{
    ptree pt;
    if (color == Colors::Auto)
    {
        pt.put("<xmlattr>.Auto", true);
        return pt;
    }

    pt.put("Red", GetRValue(color));
    pt.put("Green", GetGValue(color));
    pt.put("Blue", GetBValue(color));
    return pt;
}

COLORREF MakeColor(const ptree& pt)
{
    if (pt.get<bool>("<xmlattr>.Auto", false))
    {
        return Colors::Auto;
    }

    auto red = pt.get<int>("Red");
    auto green = pt.get<int>("Green");
    auto blue = pt.get<int>("Blue");
    return RGB(red, green, blue);
}

ptree MakePTree(const Filter& filter)
{
    ptree pt;
    pt.put("Enable", filter.enable);
    pt.put("Text", filter.text);
    pt.put("MatchType", MatchTypeToString(filter.matchType));
    pt.put("FilterType", FilterTypeToString(filter.filterType));
    pt.put_child("BackColor", MakePTree(filter.bgColor));
    pt.put_child("TextColor", MakePTree(filter.fgColor));
    return pt;
}

Filter MakeFilter(const ptree& pt)
{
    return MakeFilter(pt.get<std::string>("Text"), StringToMatchType(pt.get<std::string>("MatchType")), StringToFilterType(pt.get<std::string>("FilterType")), MakeColor(pt.get_child("BackColor")), MakeColor(pt.get_child("TextColor")), pt.get<bool>("Enable"));
}

ptree MakePTree(const std::vector<Filter>& filters)
{
    ptree pt;
    for (auto& filter : filters)
    {
        pt.add_child("Filter", MakePTree(filter));
    }
    return pt;
}

std::vector<Filter> MakeFilters(const ptree& pt)
{
    std::vector<Filter> filters;
    for (auto& item : pt)
    {
        if (item.first == "MessageFilter" ||
            item.first == "ProcessFilter" ||
            item.first == "Filter")
        {
            filters.push_back(MakeFilter(item.second));
        }
    }
    return filters;
}

FilterData LoadFilterFile(const std::filesystem::path& fileName, std::string& message)
{
    message.clear();
    const auto text = ReadTextFile(fileName);
    const auto format = DetectConfigFormat(text);
    switch (format)
    {
    case ConfigFormat::Json: return FilterDataFromJson(text);
    case ConfigFormat::LegacyXml:
    case ConfigFormat::LegacyJson:
    {
        auto data = format == ConfigFormat::LegacyXml ? FilterDataFromLegacyXml(text) : FilterDataFromLegacyJson(text);
        auto target = fileName;
        message = ConvertLegacyFile(fileName, target.replace_extension(".json"), FilterDataToJson(data));
        return data;
    }
    default: throw std::runtime_error("this is not a DebugView++ filter file");
    }
}

void SaveFilterFile(const std::filesystem::path& fileName, const std::string& name, const LogFilter& filter)
{
    WriteTextFile(fileName, FilterDataToJson(FilterData{name, filter}));
}

} // namespace debugviewpp
} // namespace fusion
