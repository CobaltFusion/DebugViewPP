// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

// The JSON representation of filters, shared by the filter files and the session (.dbconf) files.
// This header includes nlohmann/json.hpp, only include it in .cpp files. The public interface is in LogFilter.h and SessionConfig.h.

#include <nlohmann/json.hpp>
#include <vector>
#include "Filter.h"

namespace fusion {
namespace debugviewpp {

// A filter is stored as
//   {"enable": true, "text": "error", "matchType": "Simple", "filterType": "Include", "backColor": {"r": 255, "g": 255, "b": 255}, "textColor": {"auto": true}}
// A color is {"r", "g", "b"} (0-255) or {"auto": true}.
nlohmann::json ToJson(const Filter& filter);
nlohmann::json ToJson(const std::vector<Filter>& filters);

// "text", "matchType" and "filterType" are required, the rest has a default: enabled, white background, black text.
// Throws std::runtime_error (with a message that names the problem) on malformed input.
Filter FilterFromJson(const nlohmann::json& json);
std::vector<Filter> FiltersFromJson(const nlohmann::json& json);

} // namespace debugviewpp
} // namespace fusion
