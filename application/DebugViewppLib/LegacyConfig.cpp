// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <charconv>
#include <stdexcept>
#include <pugixml.hpp>
#include <nlohmann/json.hpp>
#include "CobaltFusion/StringUtil.h"
#include "DebugViewppLib/Colors.h"
#include "DebugViewppLib/LegacyConfig.h"

namespace fusion {
namespace debugviewpp {

namespace {

constexpr char Whitespace[] = " \t\r\n";

std::string_view Trimmed(std::string_view s)
{
    const auto begin = s.find_first_not_of(Whitespace);
    if (begin == std::string_view::npos)
    {
        return {};
    }
    const auto end = s.find_last_not_of(Whitespace);
    return s.substr(begin, end - begin + 1);
}

std::string_view WithoutBom(std::string_view text)
{
    if (text.starts_with("\xEF\xBB\xBF"))
    {
        text.remove_prefix(3);
    }
    return text;
}

void AddElement(const pugi::xml_node& element, LegacyNode& node)
{
    if (element.first_attribute())
    {
        LegacyNode attributes;
        for (const auto& attribute : element.attributes())
        {
            attributes.children.emplace_back(attribute.name(), LegacyNode{attribute.value(), {}});
        }
        node.children.emplace_back("<xmlattr>", std::move(attributes));
    }

    for (const auto& child : element.children())
    {
        switch (child.type())
        {
        case pugi::node_element:
        {
            LegacyNode childNode;
            AddElement(child, childNode);
            node.children.emplace_back(child.name(), std::move(childNode));
            break;
        }
        case pugi::node_pcdata:
        case pugi::node_cdata: node.value += child.value(); break;
        default: break;
        }
    }
}

// Collects a JSON document including repeated keys, in the order in which they are in the text.
class LegacyJsonHandler : public nlohmann::json::json_sax_t
{
public:
    LegacyJsonHandler()
    {
        m_stack.push_back(&m_root);
    }

    LegacyNode& Root()
    {
        return m_root;
    }

    bool null() override
    {
        return Add({});
    }

    bool boolean(bool value) override
    {
        return Add(value ? "true" : "false");
    }

    bool number_integer(number_integer_t value) override
    {
        return Add(std::to_string(value));
    }

    bool number_unsigned(number_unsigned_t value) override
    {
        return Add(std::to_string(value));
    }

    bool number_float(number_float_t /*value*/, const string_t& text) override
    {
        return Add(text);
    }

    bool string(string_t& value) override
    {
        return Add(value);
    }

    bool binary(binary_t& /*value*/) override
    {
        return false;
    }

    bool start_object(std::size_t /*elements*/) override
    {
        return Push();
    }

    bool end_object() override
    {
        m_stack.pop_back();
        return true;
    }

    bool start_array(std::size_t /*elements*/) override
    {
        return Push();
    }

    bool end_array() override
    {
        m_stack.pop_back();
        return true;
    }

    bool key(string_t& value) override
    {
        m_key = value;
        return true;
    }

    bool parse_error(std::size_t /*position*/, const std::string& /*lastToken*/, const nlohmann::json::exception& ex) override
    {
        throw std::runtime_error(std::string("invalid JSON: ") + ex.what());
    }

private:
    // arrays have no keys (the property_tree way of writing an array is children with an empty key)
    bool Push()
    {
        if (!m_started)
        {
            // the top level object is the root itself
            m_started = true;
            m_stack.push_back(&m_root);
            return true;
        }

        auto& parent = *m_stack.back();
        parent.children.emplace_back(std::move(m_key), LegacyNode());
        m_key.clear();
        m_stack.push_back(&parent.children.back().second);
        return true;
    }

    bool Add(std::string value)
    {
        auto& parent = *m_stack.back();
        parent.children.emplace_back(std::move(m_key), LegacyNode{std::move(value), {}});
        m_key.clear();
        return true;
    }

    LegacyNode m_root;
    bool m_started = false;
    std::vector<LegacyNode*> m_stack;
    std::string m_key;
};

COLORREF ColorFromLegacy(const LegacyNode& node)
{
    if (node.GetBool("<xmlattr>.Auto", false))
    {
        return Colors::Auto;
    }
    return RGB(node.GetInt("Red"), node.GetInt("Green"), node.GetInt("Blue"));
}

Filter FilterFromLegacy(const LegacyNode& node)
{
    return MakeFilter(
        node.GetString("Text"),
        StringToMatchType(node.GetString("MatchType")),
        StringToFilterType(node.GetString("FilterType")),
        ColorFromLegacy(node.Get("BackColor")),
        ColorFromLegacy(node.Get("TextColor")),
        node.GetBool("Enable"));
}

} // namespace

const LegacyNode* LegacyNode::Find(const std::string& key) const
{
    for (const auto& child : children)
    {
        if (child.first == key)
        {
            return &child.second;
        }
    }
    return nullptr;
}

const LegacyNode* LegacyNode::TryGet(const std::string& path) const
{
    const LegacyNode* node = this;
    size_t begin = 0;
    while (node != nullptr)
    {
        const auto end = path.find('.', begin);
        node = node->Find(path.substr(begin, end == std::string::npos ? end : end - begin));
        if (end == std::string::npos)
        {
            break;
        }
        begin = end + 1;
    }
    return node;
}

const LegacyNode& LegacyNode::Get(const std::string& path) const
{
    const auto* node = TryGet(path);
    if (node == nullptr)
    {
        throw std::runtime_error("'" + path + "' is missing");
    }
    return *node;
}

std::string LegacyNode::GetString(const std::string& path) const
{
    return Get(path).value;
}

int LegacyNode::AsInt() const
{
    const auto text = Trimmed(value);
    int result = 0;
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), result);
    if (text.empty() || ec != std::errc() || end != text.data() + text.size())
    {
        throw std::runtime_error("'" + value + "' is not a number");
    }
    return result;
}

bool LegacyNode::AsBool() const
{
    const auto text = Trimmed(value);
    if (text == "true" || text == "1")
    {
        return true;
    }
    if (text == "false" || text == "0")
    {
        return false;
    }
    throw std::runtime_error("'" + value + "' is not true or false");
}

int LegacyNode::GetInt(const std::string& path) const
{
    return Get(path).AsInt();
}

bool LegacyNode::GetBool(const std::string& path) const
{
    return Get(path).AsBool();
}

bool LegacyNode::GetBool(const std::string& path, bool defaultValue) const
{
    const auto* node = TryGet(path);
    return node == nullptr ? defaultValue : node->AsBool();
}

LegacyNode ParseLegacyXml(const std::string& text)
{
    const auto data = WithoutBom(text);
    pugi::xml_document document;
    // parse_ws_pcdata_single keeps the text of an element that only has white space (a filter of a single space), the indentation is dropped
    const auto result = document.load_buffer(data.data(), data.size(), pugi::parse_default | pugi::parse_ws_pcdata_single, pugi::encoding_utf8);
    if (!result)
    {
        throw std::runtime_error(std::string("invalid XML: ") + result.description());
    }

    LegacyNode root;
    AddElement(document, root);
    return root;
}

LegacyNode ParseLegacyJson(const std::string& text)
{
    const auto data = WithoutBom(text);
    LegacyJsonHandler handler;
    if (!nlohmann::json::sax_parse(data.begin(), data.end(), &handler))
    {
        throw std::runtime_error("invalid JSON");
    }
    return std::move(handler.Root());
}

std::vector<Filter> FiltersFromLegacy(const LegacyNode& node)
{
    std::vector<Filter> filters;
    for (const auto& child : node.children)
    {
        if (child.first == "MessageFilter" || child.first == "ProcessFilter" || child.first == "Filter")
        {
            filters.push_back(FilterFromLegacy(child.second));
        }
    }
    return filters;
}

FilterData FilterDataFromLegacy(const LegacyNode& document)
{
    FilterData data;
    data.name = document.GetString("Filter.Name");
    data.filter.messageFilters = FiltersFromLegacy(document.Get("Filter.MessageFilters"));
    data.filter.processFilters = FiltersFromLegacy(document.Get("Filter.ProcessFilters"));
    return data;
}

FilterData FilterDataFromLegacyXml(const std::string& text)
{
    return FilterDataFromLegacy(ParseLegacyXml(text));
}

FilterData FilterDataFromLegacyJson(const std::string& text)
{
    return FilterDataFromLegacy(ParseLegacyJson(text));
}

} // namespace debugviewpp
} // namespace fusion
