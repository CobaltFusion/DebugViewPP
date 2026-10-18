// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include "CobaltFusion/Str.h"
#include "DebugViewppLib/ConfigFile.h"

namespace fusion {
namespace debugviewpp {

namespace fs = std::filesystem;

namespace {

std::string Describe(const fs::path& path)
{
    return "'" + Str(path.wstring()).str() + "'";
}

fs::path ArchiveName(const fs::path& file)
{
    fs::path candidate = file;
    candidate += ".archived";
    for (int i = 2; fs::exists(candidate); ++i)
    {
        candidate = file;
        candidate += ".archived" + std::to_string(i);
    }
    return candidate;
}

} // namespace

ConfigFormat DetectConfigFormat(const std::string& text)
{
    auto begin = text.find_first_not_of(" \t\r\n");
    if (begin != std::string::npos && text.compare(begin, 3, "\xEF\xBB\xBF") == 0)
    {
        begin = text.find_first_not_of(" \t\r\n", begin + 3);
    }
    if (begin == std::string::npos)
    {
        return ConfigFormat::Unknown;
    }

    if (text[begin] == '<')
    {
        return ConfigFormat::LegacyXml;
    }
    if (text[begin] != '{')
    {
        return ConfigFormat::Unknown;
    }

    // property_tree wrote the top level object with the name of the document in it, the current format has a version
    const auto json = nlohmann::json::parse(text.begin() + begin, text.end(), nullptr, false);
    if (json.is_object() && json.contains("version"))
    {
        return ConfigFormat::Json;
    }
    if (json.is_discarded())
    {
        return ConfigFormat::Unknown;
    }
    return json.is_object() && (json.contains("Filter") || json.contains("DebugViewPP")) ? ConfigFormat::LegacyJson : ConfigFormat::Unknown;
}

std::string ReadTextFile(const fs::path& fileName)
{
    std::ifstream stream(fileName, std::ios::binary);
    if (!stream)
    {
        throw std::runtime_error("cannot open " + Describe(fileName));
    }
    std::string text((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    if (stream.bad())
    {
        throw std::runtime_error("cannot read " + Describe(fileName));
    }
    return text;
}

void WriteTextFile(const fs::path& fileName, const std::string& text)
{
    fs::path temporary = fileName;
    temporary += ".tmp";
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        stream.write(text.data(), static_cast<std::streamsize>(text.size()));
        stream.close();
        if (!stream)
        {
            std::error_code ignored;
            fs::remove(temporary, ignored);
            throw std::runtime_error("cannot write " + Describe(fileName));
        }
    }

    std::error_code ec;
    fs::rename(temporary, fileName, ec); // replaces an existing file
    if (ec)
    {
        std::error_code ignored;
        fs::remove(temporary, ignored);
        throw std::runtime_error("cannot write " + Describe(fileName) + ": " + ec.message());
    }
}

std::string ConvertLegacyFile(const fs::path& legacyFile, const fs::path& target, const std::string& jsonText)
{
    std::error_code ec;
    const bool sameFile = fs::equivalent(legacyFile, target, ec);
    if (!sameFile && fs::exists(target))
    {
        return Describe(target) + " exists already, " + Describe(legacyFile) + " was not converted.";
    }

    fs::path temporary = target;
    temporary += ".new";
    try
    {
        WriteTextFile(temporary, jsonText);
    }
    catch (const std::exception& ex)
    {
        return std::string("Could not convert ") + Describe(legacyFile) + " to JSON: " + ex.what();
    }

    const fs::path archive = ArchiveName(legacyFile);
    fs::rename(legacyFile, archive, ec);
    if (ec)
    {
        std::error_code ignored;
        fs::remove(temporary, ignored);
        return "Could not convert " + Describe(legacyFile) + " to JSON, it can not be renamed: " + ec.message();
    }

    fs::rename(temporary, target, ec);
    if (ec)
    {
        // put the original back, nothing is lost
        std::error_code ignored;
        fs::rename(archive, legacyFile, ignored);
        fs::remove(temporary, ignored);
        return "Could not convert " + Describe(legacyFile) + " to JSON: " + ec.message();
    }

    return Describe(legacyFile) + " was converted to JSON (" + Describe(target) + "). The original is kept as " + Describe(archive) + ".";
}

} // namespace debugviewpp
} // namespace fusion
