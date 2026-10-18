// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>
#include "LogFilter.h"
#include "SourceType.h"

namespace fusion {
namespace debugviewpp {

// The session: what File, Save Configuration saves in a .dbconf file, and what is loaded from one. The settings that are kept in the
// registry are not part of it. The text is UTF-8.

struct ColumnConfig
{
    int index = 0; // the Column::type
    std::optional<bool> enable;
    std::optional<int> width;
    std::optional<int> order;
};

struct ViewConfig
{
    std::string name;
    bool clockTime = true;
    bool processColors = false;
    LogFilter filters;
    std::optional<std::vector<ColumnConfig>> columns; // files of old versions may not have them
};

struct SourceConfig
{
    bool enabled = true;
    std::string description;
    SourceType::type type = SourceType::System;
    std::string address;
    int port = 0;
};

struct SessionConfig
{
    bool autoNewline = false;
    bool linkViews = false;
    std::vector<ViewConfig> views;     // in the order of the tabs
    std::vector<SourceConfig> sources; // in the order of the sources dialog
};

// The JSON format, version 2:
//   {"version": 2, "autoNewline": false, "linkViews": true,
//    "views": [{"name": "...", "clockTime": true, "processColors": false, "messageFilters": [...], "processFilters": [...],
//               "columns": [{"index": 0, "enable": true, "width": 20, "order": 0}, ...]}],
//    "sources": [{"enabled": true, "description": "...", "type": "UDP Socket", "address": "0.0.0.0", "port": 2021}]}
// SessionConfigFromJson and SessionConfigFromLegacyXml throw std::runtime_error when the text is not valid.
std::string SessionConfigToJson(const SessionConfig& config);
SessionConfig SessionConfigFromJson(const std::string& text);

// The XML of the .dbconf files that older versions wrote (see LegacyConfig.h).
SessionConfig SessionConfigFromLegacyXml(const std::string& text);

// Loads a .dbconf file: JSON, or the XML of an older version. A legacy file is converted: the JSON replaces it under the same name and
// the old file is renamed to <name>.archived, see ConvertLegacyFile. 'message' tells what was converted (or why not), it is empty when
// nothing was. Throws std::runtime_error when the file can not be read.
SessionConfig LoadSessionFile(const std::filesystem::path& fileName, std::string& message);
void SaveSessionFile(const std::filesystem::path& fileName, const SessionConfig& config);

} // namespace debugviewpp
} // namespace fusion
