// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <filesystem>
#include <string>

namespace fusion {
namespace debugviewpp {

// Helpers for the configuration files (filter files and .dbconf session files). They are JSON, versions of DebugView++ before 1.22
// wrote XML, or JSON in the format of boost property_tree. The legacy files are still read, and converted to JSON when they are loaded.

enum class ConfigFormat
{
    Unknown,
    Json,       // the current format, a JSON object with a "version"
    LegacyXml,  // written by boost property_tree
    LegacyJson, // written by boost property_tree (not valid JSON in the strict sense: repeated keys)
};

// The format is detected by looking at the content, not at the extension: '<' is XML, '{' is JSON. In JSON a "version" means the
// current format.
ConfigFormat DetectConfigFormat(const std::string& text);

// Whole files as bytes (UTF-8). Both throw std::runtime_error with the name of the file when it fails.
std::string ReadTextFile(const std::filesystem::path& fileName);

// Writes to a temporary file next to the target and then replaces the target, so a failure never leaves half a file behind.
void WriteTextFile(const std::filesystem::path& fileName, const std::string& text);

// Replaces a legacy file by its JSON equivalent, without ever deleting the legacy content:
//  1. the JSON is written next to the target under a temporary name,
//  2. the legacy file is renamed to "<name>.archived" (".archived2", ".archived3" ... if that exists),
//  3. the temporary file gets the name of the target.
// The target is usually the legacy file itself (a .dbconf or a legacy JSON file), or a file with another extension (the filter.xml becomes filter.json).
// A target that exists and is not the legacy file itself is never overwritten.
// Returns a message for the user, that tells what was done or why nothing was (it does not throw). The conversion is a convenience,
// loading the file is not affected when it fails.
std::string ConvertLegacyFile(const std::filesystem::path& legacyFile, const std::filesystem::path& target, const std::string& jsonText);

} // namespace debugviewpp
} // namespace fusion
