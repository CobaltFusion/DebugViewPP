// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/


#include "stdafx.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#pragma warning(push, 3)
#include <boost/property_tree/json_parser.hpp>
#pragma warning(pop)
#include "LogFilter.h"

namespace fusion {
namespace debugviewpp {

using boost::property_tree::ptree;

ptree MakePTree(COLORREF color)
{
	ptree pt;
	pt.put("Red", GetRValue(color));
	pt.put("Green", GetGValue(color));
	pt.put("Blue", GetBValue(color));
	return pt;
}

COLORREF MakeColor(const ptree& pt)
{
	auto red = pt.get<int>("Red");
	auto green = pt.get<int>("Green");
	auto blue = pt.get<int>("Blue");
	return RGB(red, green, blue);
}

ptree MakePTree(const MessageFilter& filter)
{
	ptree pt;
	pt.put("Enable", filter.enable);
	pt.put("Text", filter.text);
	pt.put("Type", FilterTypeToString(filter.type));
	pt.put_child("BackColor", MakePTree(filter.bgColor));
	pt.put_child("TextColor", MakePTree(filter.fgColor));
	return pt;
}

MessageFilter MakeMessageFilter(const ptree& pt)
{
	MessageFilter filter;
	filter.enable = pt.get<bool>("Enable");
	filter.text = pt.get<std::string>("Text");
	filter.type = StringToFilterType(pt.get<std::string>("Type"));
	filter.bgColor = MakeColor(pt.get_child("BackColor"));
	filter.fgColor = MakeColor(pt.get_child("TextColor"));
	return filter;
}

ptree MakePTree(const std::vector<MessageFilter>& filters)
{
	ptree pt;
	for (auto it = filters.begin(); it != filters.end(); ++it)
		pt.add_child("MessageFilter", MakePTree(*it));
	return pt;
}

std::vector<MessageFilter> MakeMessageFilters(const ptree& pt)
{
	std::vector<MessageFilter> filters;
	for (auto it = pt.begin(); it != pt.end(); ++it)
	{
		if (it->first == "MessageFilter")
			filters.push_back(MakeMessageFilter(it->second));
	}
	return filters;
}

ptree MakePTree(const ProcessFilter& filter)
{
	ptree pt;
	pt.put("Enable", filter.enable);
	pt.put("Text", filter.text);
	pt.put("Type", FilterTypeToInt(filter.type));
	pt.put_child("BackColor", MakePTree(filter.bgColor));
	pt.put_child("TextColor", MakePTree(filter.fgColor));
	return pt;
}

ProcessFilter MakeProcessFilter(const ptree& pt)
{
	ProcessFilter filter;
	filter.enable = pt.get<bool>("Enable");
	filter.text = pt.get<std::string>("Text");
	filter.type = StringToFilterType(pt.get<std::string>("Type"));
	filter.bgColor = MakeColor(pt.get_child("BackColor"));
	filter.fgColor = MakeColor(pt.get_child("TextColor"));
	return filter;
}

ptree MakePTree(const std::vector<ProcessFilter>& filters)
{
	ptree pt;
	for (auto it = filters.begin(); it != filters.end(); ++it)
		pt.add_child("Filter", MakePTree(*it));
	return pt;
}

std::vector<ProcessFilter> MakeProcessFilters(const ptree& pt)
{
	std::vector<ProcessFilter> filters;
	for (auto it = pt.begin(); it != pt.end(); ++it)
	{
		if (it->first == "ProcessFilter")
			filters.push_back(MakeProcessFilter(it->second));
	}
	return filters;
}

ptree MakePTree(const std::string& name, const LogFilter& filter)
{
	ptree pt;
	pt.put("Filter.Name", name);
	pt.put_child("Filter.MessageFilters", MakePTree(filter.messageFilters));
	pt.put_child("Filter.ProcessFilters", MakePTree(filter.processFilters));
	return pt;
}

FilterData MakeFilterData(const ptree& pt)
{
	FilterData data;
	data.name = pt.get<std::string>("Filter.Name");
	data.filter.messageFilters = MakeMessageFilters(pt.get_child("Filter.MessageFilters"));
	data.filter.processFilters = MakeProcessFilters(pt.get_child("Filter.ProcessFilters"));

	return data;
}

void SaveXml(const std::string& fileName, const std::string& name, const LogFilter& filter)
{
	boost::property_tree::xml_writer_settings<char> settings('\t', 1);
	write_xml(fileName, MakePTree(name, filter), std::locale(), settings);
}

void SaveJson(const std::string& fileName, const std::string& name, const LogFilter& filter)
{
	write_json(fileName, MakePTree(name, filter));
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
