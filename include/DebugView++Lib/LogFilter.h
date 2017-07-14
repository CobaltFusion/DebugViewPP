// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

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

void SaveXml(const std::string& fileName, const std::string& name, const LogFilter& filter);
void SaveJson(const std::string& fileName, const std::string& name, const LogFilter& filter);
FilterData LoadXml(const std::string& fileName);
FilterData LoadJson(const std::string& fileName);

} // namespace debugviewpp
} // namespace fusion
