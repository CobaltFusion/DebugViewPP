// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <utility>
#include <vector>
#include "LogFilter.h"

namespace fusion {
namespace debugviewpp {

// Readers for the files that older versions wrote with boost property_tree: .dbconf session files and filter files, as XML or as
// JSON. Files like these exist on the machines of users, so they stay readable (see DebugViewppTest/testdata/README.md).
// New files are always written as JSON (see FilterJson.h, SessionConfig.h), these readers are only used to convert old files.
//
// Both formats are first read into a LegacyNode tree, which has the shape of the property_tree the old code used, so the
// mapping to the structs follows the old code:
//  - the XML attributes of an element are the children of a child named "<xmlattr>" (<BackColor Auto="true"/> has the path "BackColor.<xmlattr>.Auto")
//  - the legacy JSON is not valid JSON in the strict sense: a list is an object with the same key repeated ("Filter": {...}, "Filter": {...}),
//    and every value is a string. The repeated keys are kept, in order.
class LegacyNode
{
public:
    std::string value;
    std::vector<std::pair<std::string, LegacyNode>> children;

    // The first child with this key, or nullptr.
    const LegacyNode* Find(const std::string& key) const;

    // A child by path ("Filter.Name"). Throws std::runtime_error when it is not there.
    const LegacyNode& Get(const std::string& path) const;
    const LegacyNode* TryGet(const std::string& path) const;

    // Values are converted like boost did: a bool is "true"/"false" (or 1/0), surrounding whitespace is ignored.
    std::string GetString(const std::string& path) const;
    bool GetBool(const std::string& path) const;
    bool GetBool(const std::string& path, bool defaultValue) const;
    int GetInt(const std::string& path) const;
    int AsInt() const;
    bool AsBool() const;
};

// Both throw std::runtime_error when the text is not valid XML / JSON.
LegacyNode ParseLegacyXml(const std::string& text);
LegacyNode ParseLegacyJson(const std::string& text);

// The filters in a node whose children are the filters (named "Filter", "MessageFilter" or "ProcessFilter").
std::vector<Filter> FiltersFromLegacy(const LegacyNode& node);

// A filter file (Filter.Name, Filter.MessageFilters, Filter.ProcessFilters).
FilterData FilterDataFromLegacy(const LegacyNode& document);
FilterData FilterDataFromLegacyXml(const std::string& text);
FilterData FilterDataFromLegacyJson(const std::string& text);

} // namespace debugviewpp
} // namespace fusion
