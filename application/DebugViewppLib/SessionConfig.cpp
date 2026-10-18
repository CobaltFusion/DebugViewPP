// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <stdexcept>
#include "DebugViewppLib/ConfigFile.h"
#include "DebugViewppLib/FilterJson.h"
#include "DebugViewppLib/LegacyConfig.h"
#include "DebugViewppLib/SessionConfig.h"

namespace fusion {
namespace debugviewpp {

using nlohmann::json;

namespace {

constexpr int SessionFileVersion = 2;

// ---- JSON

json ToJson(const ColumnConfig& column)
{
    json result = {{"index", column.index}};
    if (column.enable)
    {
        result["enable"] = *column.enable;
    }
    if (column.width)
    {
        result["width"] = *column.width;
    }
    if (column.order)
    {
        result["order"] = *column.order;
    }
    return result;
}

json ToJson(const ViewConfig& view)
{
    json result = {
        {"name", view.name},
        {"clockTime", view.clockTime},
        {"processColors", view.processColors},
        {"messageFilters", ToJson(view.filters.messageFilters)},
        {"processFilters", ToJson(view.filters.processFilters)},
    };
    if (view.columns)
    {
        json columns = json::array();
        for (const auto& column : *view.columns)
        {
            columns.push_back(ToJson(column));
        }
        result["columns"] = columns;
    }
    return result;
}

json ToJson(const SourceConfig& source)
{
    return {
        {"enabled", source.enabled},
        {"description", source.description},
        {"type", SourceTypeToString(source.type)},
        {"address", source.address},
        {"port", source.port},
    };
}

template <typename T>
T Required(const json& object, const char* name, const char* what)
{
    auto it = object.find(name);
    if (it == object.end())
    {
        throw std::runtime_error(std::string(what) + ": '" + name + "' is missing");
    }
    try
    {
        return it->get<T>();
    }
    catch (const json::exception&)
    {
        throw std::runtime_error(std::string(what) + ": '" + name + "' has the wrong type");
    }
}

template <typename T>
T Optional(const json& object, const char* name, const char* what, T defaultValue)
{
    return object.contains(name) ? Required<T>(object, name, what) : defaultValue;
}

template <typename T>
std::optional<T> OptionalValue(const json& object, const char* name, const char* what)
{
    if (!object.contains(name))
    {
        return std::nullopt;
    }
    return Required<T>(object, name, what);
}

const json& ObjectAt(const json& array, size_t i, const char* what)
{
    if (!array[i].is_object())
    {
        throw std::runtime_error(std::string(what) + ": an object is expected");
    }
    return array[i];
}

ColumnConfig ColumnFromJson(const json& json)
{
    ColumnConfig column;
    column.index = Required<int>(json, "index", "column");
    column.enable = OptionalValue<bool>(json, "enable", "column");
    column.width = OptionalValue<int>(json, "width", "column");
    column.order = OptionalValue<int>(json, "order", "column");
    return column;
}

ViewConfig ViewFromJson(const json& json)
{
    ViewConfig view;
    view.name = Required<std::string>(json, "name", "view");
    view.clockTime = Optional<bool>(json, "clockTime", "view", true);
    view.processColors = Optional<bool>(json, "processColors", "view", false);
    if (json.contains("messageFilters"))
    {
        view.filters.messageFilters = FiltersFromJson(json["messageFilters"]);
    }
    if (json.contains("processFilters"))
    {
        view.filters.processFilters = FiltersFromJson(json["processFilters"]);
    }
    if (json.contains("columns"))
    {
        const auto& columns = json["columns"];
        if (!columns.is_array())
        {
            throw std::runtime_error("view: 'columns' must be an array");
        }
        view.columns.emplace();
        for (size_t i = 0; i < columns.size(); ++i)
        {
            view.columns->push_back(ColumnFromJson(ObjectAt(columns, i, "column")));
        }
    }
    return view;
}

SourceConfig SourceFromJson(const json& json)
{
    SourceConfig source;
    source.enabled = Optional<bool>(json, "enabled", "source", true);
    source.description = Required<std::string>(json, "description", "source");
    source.type = StringToSourceType(Required<std::string>(json, "type", "source"));
    source.address = Optional<std::string>(json, "address", "source", std::string());
    source.port = Optional<int>(json, "port", "source", 0);
    return source;
}

// ---- the XML of older versions

ColumnConfig ColumnFromLegacy(const LegacyNode& node)
{
    ColumnConfig column;
    column.index = node.GetInt("Index");
    if (const auto* enable = node.TryGet("Enable"))
    {
        column.enable = enable->AsBool();
    }
    if (const auto* width = node.TryGet("Width"))
    {
        column.width = width->AsInt();
    }
    if (const auto* order = node.TryGet("Order"))
    {
        column.order = order->AsInt();
    }
    return column;
}

} // namespace

std::string SessionConfigToJson(const SessionConfig& config)
{
    json views = json::array();
    for (const auto& view : config.views)
    {
        views.push_back(ToJson(view));
    }

    json sources = json::array();
    for (const auto& source : config.sources)
    {
        sources.push_back(ToJson(source));
    }

    json root = {
        {"version", SessionFileVersion},
        {"autoNewline", config.autoNewline},
        {"linkViews", config.linkViews},
        {"views", views},
        {"sources", sources},
    };
    return root.dump(4);
}

SessionConfig SessionConfigFromJson(const std::string& text)
{
    const json root = json::parse(text, nullptr, false);
    if (root.is_discarded() || !root.is_object())
    {
        throw std::runtime_error("the configuration file is not valid JSON");
    }

    const auto version = root.find("version");
    if (version != root.end() && version->is_number_integer() && version->get<int>() > SessionFileVersion)
    {
        throw std::runtime_error("the configuration file was written by a newer version of DebugView++");
    }

    SessionConfig config;
    config.autoNewline = Optional<bool>(root, "autoNewline", "configuration", false);
    config.linkViews = Optional<bool>(root, "linkViews", "configuration", false);

    if (root.contains("views"))
    {
        const auto& views = root["views"];
        if (!views.is_array())
        {
            throw std::runtime_error("configuration: 'views' must be an array");
        }
        for (size_t i = 0; i < views.size(); ++i)
        {
            config.views.push_back(ViewFromJson(ObjectAt(views, i, "view")));
        }
    }

    if (root.contains("sources"))
    {
        const auto& sources = root["sources"];
        if (!sources.is_array())
        {
            throw std::runtime_error("configuration: 'sources' must be an array");
        }
        for (size_t i = 0; i < sources.size(); ++i)
        {
            config.sources.push_back(SourceFromJson(ObjectAt(sources, i, "source")));
        }
    }
    return config;
}

SessionConfig SessionConfigFromLegacyXml(const std::string& text)
{
    const auto document = ParseLegacyXml(text);
    const auto& root = document.Get("DebugViewPP");

    SessionConfig config;
    config.autoNewline = root.GetBool("AutoNewline");
    config.linkViews = root.GetBool("LinkViews");

    // the views and sources were sorted by their Index when they were loaded
    std::vector<std::pair<int, ViewConfig>> views;
    for (const auto& item : root.Get("Views").children)
    {
        if (item.first != "View")
        {
            continue;
        }

        const auto& node = item.second;
        ViewConfig view;
        view.name = node.GetString("Name");
        view.clockTime = node.GetBool("ClockTime");
        view.processColors = node.GetBool("ProcessColors");
        view.filters.messageFilters = FiltersFromLegacy(node.Get("MessageFilters"));
        view.filters.processFilters = FiltersFromLegacy(node.Get("ProcessFilters"));
        if (const auto* columns = node.TryGet("Columns"))
        {
            view.columns.emplace();
            for (const auto& column : columns->children)
            {
                // only the columns that have an index are used
                if (column.first == "Column" && column.second.TryGet("Index") != nullptr)
                {
                    view.columns->push_back(ColumnFromLegacy(column.second));
                }
            }
        }
        views.emplace_back(node.GetInt("Index"), std::move(view));
    }
    std::stable_sort(views.begin(), views.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    for (auto& view : views)
    {
        config.views.push_back(std::move(view.second));
    }

    std::vector<std::pair<int, SourceConfig>> sources;
    if (const auto* sourcesNode = root.TryGet("Sources"))
    {
        for (const auto& item : sourcesNode->children)
        {
            if (item.first != "Source")
            {
                continue;
            }

            const auto& node = item.second;
            SourceConfig source;
            source.type = StringToSourceType(node.GetString("SourceType"));
            source.description = node.GetString("Description");
            source.address = node.GetString("Address");
            source.port = node.GetInt("Port");
            source.enabled = node.GetBool("Enabled");
            sources.emplace_back(node.GetInt("Index"), std::move(source));
        }
    }
    std::stable_sort(sources.begin(), sources.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    for (auto& source : sources)
    {
        config.sources.push_back(std::move(source.second));
    }
    return config;
}

SessionConfig LoadSessionFile(const std::filesystem::path& fileName, std::string& message)
{
    message.clear();
    const auto text = ReadTextFile(fileName);
    switch (DetectConfigFormat(text))
    {
    case ConfigFormat::Json: return SessionConfigFromJson(text);
    case ConfigFormat::LegacyXml:
    {
        auto config = SessionConfigFromLegacyXml(text);
        message = ConvertLegacyFile(fileName, fileName, SessionConfigToJson(config));
        return config;
    }
    default: throw std::runtime_error("this is not a DebugView++ configuration file");
    }
}

void SaveSessionFile(const std::filesystem::path& fileName, const SessionConfig& config)
{
    WriteTextFile(fileName, SessionConfigToJson(config));
}

} // namespace debugviewpp
} // namespace fusion
