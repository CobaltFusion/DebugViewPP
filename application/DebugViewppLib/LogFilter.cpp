// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <stdexcept>
#include "DebugViewppLib/ConfigFile.h"
#include "DebugViewppLib/LegacyConfig.h"
#include "DebugViewppLib/LogFilter.h"

namespace fusion {
namespace debugviewpp {

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
