// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <boost/property_tree/ptree_fwd.hpp>
#include "Filter.h"

namespace fusion {
namespace debugviewpp {

struct FilterData
{
    std::string name;
    LogFilter filter;
};

boost::property_tree::ptree MakePTree(const std::vector<Filter>& filters);
std::vector<Filter> MakeFilters(const boost::property_tree::ptree& pt);

// The JSON text of the filter file format (the version 2 format, see FilterJson.h):
//   {"version": 2, "name": "...", "messageFilters": [ ... ], "processFilters": [ ... ]}
// FilterDataFromJson throws std::runtime_error when the text is not valid, or was written by a newer version.
std::string FilterDataToJson(const FilterData& data);
FilterData FilterDataFromJson(const std::string& text);

void SaveXml(const std::string& fileName, const std::string& name, const LogFilter& filter);
void SaveJson(const std::string& fileName, const std::string& name, const LogFilter& filter);
FilterData LoadXml(const std::string& fileName);
FilterData LoadJson(const std::string& fileName);

} // namespace debugviewpp
} // namespace fusion
