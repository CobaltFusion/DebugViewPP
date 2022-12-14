// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/


// #include "stdafx.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#pragma warning(push, 3)
#include <boost/property_tree/json_parser.hpp>
#pragma warning(pop)
#include "DebugView++Lib/Colors.h"
#include "DebugView++Lib/LogFilter.h"

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

ptree MakePTree(const FilterData& view)
{
    ptree pt;
    pt.put("Filter.Name", view.name);
    pt.put_child("Filter.MessageFilters", MakePTree(view.filter.messageFilters));
    pt.put_child("Filter.ProcessFilters", MakePTree(view.filter.processFilters));
    return pt;
}

FilterData MakeFilterData(const ptree& pt)
{
    FilterData data;
    data.name = pt.get<std::string>("Filter.Name");
    data.filter.messageFilters = MakeFilters(pt.get_child("Filter.MessageFilters"));
    data.filter.processFilters = MakeFilters(pt.get_child("Filter.ProcessFilters"));

    return data;
}

void SaveXml(const std::string& fileName, const std::string& name, const LogFilter& filter)
{
    boost::property_tree::xml_writer_settings<std::string> settings('\t', 1);
    FilterData view = {name, filter};
    write_xml(fileName, MakePTree(view), std::locale(), settings);
}

void SaveJson(const std::string& fileName, const std::string& name, const LogFilter& filter)
{
    FilterData view = {name, filter};
    write_json(fileName, MakePTree(view));
}

FilterData LoadXml(const std::string& fileName)
{
    ptree pt;
    read_xml(fileName, pt, boost::property_tree::xml_parser::trim_whitespace);
    return MakeFilterData(pt);
}

FilterData LoadJson(const std::string& fileName)
{
    ptree pt;
    read_json(fileName, pt);
    return MakeFilterData(pt);
}

} // namespace debugviewpp
} // namespace fusion
