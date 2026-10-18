// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <filesystem>
#include "Filter.h"

namespace fusion {
namespace debugviewpp {

struct FilterData
{
    std::string name;
    LogFilter filter;
};

// The JSON text of the filter file format (the version 2 format, see FilterJson.h):
//   {"version": 2, "name": "...", "messageFilters": [ ... ], "processFilters": [ ... ]}
// FilterDataFromJson throws std::runtime_error when the text is not valid, or was written by a newer version.
std::string FilterDataToJson(const FilterData& data);
FilterData FilterDataFromJson(const std::string& text);

// Filter files are JSON. Files that older versions wrote (XML, or JSON of property_tree) are still read: they are converted, see
// ConvertLegacyFile: the old file is renamed to <name>.archived and the JSON is written next to it as <name>.json.
// 'message' tells what was converted (or why not), it is empty when nothing was converted. Throws std::runtime_error when the file can not be read.
FilterData LoadFilterFile(const std::filesystem::path& fileName, std::string& message);
void SaveFilterFile(const std::filesystem::path& fileName, const std::string& name, const LogFilter& filter);

} // namespace debugviewpp
} // namespace fusion
