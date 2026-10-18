// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <pcre2.h>
#include <algorithm>
#include <new>
#include "DebugViewppLib/Regex.h"

namespace fusion {
namespace debugviewpp {

namespace {

constexpr size_t npos = std::string_view::npos;

// the most groups that are reported (the whole match is group 0), a pattern can have more, those are not reported
constexpr uint32_t MaxGroups = 128;

// A match that needs more steps than this is a pattern that backtracks without end, it counts as 'no match'. The default is 10 million.
constexpr uint32_t MatchLimit = 1'000'000;

// The match data and context are not shared, a search must not allocate and a Regex is used from more than one thread.
struct ThreadData
{
    pcre2_match_data* data = nullptr;
    pcre2_match_context* context = nullptr;

    ThreadData()
    {
        data = pcre2_match_data_create(MaxGroups, nullptr);
        context = pcre2_match_context_create(nullptr);
        if (data == nullptr || context == nullptr)
        {
            throw std::bad_alloc();
        }
        pcre2_set_match_limit(context, MatchLimit);
    }

    ~ThreadData()
    {
        pcre2_match_context_free(context);
        pcre2_match_data_free(data);
    }

    ThreadData(const ThreadData&) = delete;
    ThreadData& operator=(const ThreadData&) = delete;
};

ThreadData& GetThreadData()
{
    thread_local ThreadData threadData;
    return threadData;
}

std::string ErrorMessage(int code)
{
    PCRE2_UCHAR buffer[256];
    if (pcre2_get_error_message(code, buffer, sizeof(buffer)) < 0)
    {
        return "invalid regular expression";
    }
    return reinterpret_cast<const char*>(buffer);
}

} // namespace

RegexError::RegexError(const std::string& message, size_t offset) :
    std::runtime_error(message),
    m_offset(offset)
{
}

size_t RegexError::offset() const
{
    return m_offset;
}

size_t RegexMatch::size() const
{
    return groups.size();
}

bool RegexMatch::Matched(size_t group) const
{
    return group < groups.size() && groups[group].first != npos;
}

std::string_view RegexMatch::Group(size_t group) const
{
    return Matched(group) ? text.substr(groups[group].first, groups[group].second) : std::string_view();
}

size_t RegexMatch::Position(size_t group) const
{
    return group < groups.size() ? groups[group].first : npos;
}

size_t RegexMatch::Length(size_t group) const
{
    return Matched(group) ? groups[group].second : 0;
}

size_t RegexMatch::End() const
{
    return Matched(0) ? groups[0].first + groups[0].second : npos;
}

struct Regex::Code
{
    pcre2_code* code = nullptr;
    size_t groupCount = 1; // the whole match and the capture groups

    ~Code()
    {
        pcre2_code_free(code);
    }
};

Regex::Regex() = default;

Regex::Regex(const std::string& pattern, bool ignoreCase)
{
    int errorCode = 0;
    PCRE2_SIZE errorOffset = 0;
    // PCRE2_DOLLAR_ENDONLY: '$' is the end of the text, not also before a newline at the end, like in ECMAScript
    const uint32_t options = PCRE2_DOLLAR_ENDONLY | (ignoreCase ? PCRE2_CASELESS : 0);
    auto code = std::make_shared<Code>();
    code->code = pcre2_compile(reinterpret_cast<PCRE2_SPTR>(pattern.data()), pattern.size(), options, &errorCode, &errorOffset, nullptr);
    if (code->code == nullptr)
    {
        throw RegexError(ErrorMessage(errorCode) + " (at position " + std::to_string(errorOffset) + ")", errorOffset);
    }

    // without the JIT (or when it can not compile this pattern) pcre2_match interprets the pattern, that works the same but slower
    pcre2_jit_compile(code->code, PCRE2_JIT_COMPLETE);

    uint32_t captureCount = 0;
    pcre2_pattern_info(code->code, PCRE2_INFO_CAPTURECOUNT, &captureCount);
    code->groupCount = std::min<size_t>(captureCount + 1, MaxGroups);
    m_code = std::move(code);
}

Regex::~Regex() = default;

bool Regex::Search(std::string_view text) const
{
    RegexMatch ignored;
    return Search(text, ignored, 0);
}

bool Regex::Search(std::string_view text, RegexMatch& match, size_t offset) const
{
    match.text = text;
    match.groups.clear();

    if (!m_code)
    {
        // the empty pattern
        if (offset > text.size())
        {
            return false;
        }
        match.groups.emplace_back(offset, 0);
        return true;
    }

    auto& threadData = GetThreadData();
    const char* subject = text.empty() ? "" : text.data();
    int result = pcre2_match(m_code->code, reinterpret_cast<PCRE2_SPTR>(subject), text.size(), offset, 0, threadData.data, threadData.context);
    if (result == PCRE2_ERROR_JIT_STACKLIMIT)
    {
        // a pattern that needs more stack than the JIT has, the interpreter has more
        result = pcre2_match(m_code->code, reinterpret_cast<PCRE2_SPTR>(subject), text.size(), offset, PCRE2_NO_JIT, threadData.data, threadData.context);
    }
    if (result < 0)
    {
        return false; // no match, or the match limit was reached
    }

    const PCRE2_SIZE* vector = pcre2_get_ovector_pointer(threadData.data);
    match.groups.reserve(m_code->groupCount);
    for (size_t i = 0; i < m_code->groupCount; ++i)
    {
        const PCRE2_SIZE begin = vector[2 * i];
        const PCRE2_SIZE end = vector[2 * i + 1];
        if (begin == PCRE2_UNSET || end == PCRE2_UNSET)
        {
            match.groups.emplace_back(npos, 0);
        }
        else
        {
            match.groups.emplace_back(begin, end - begin);
        }
    }
    return true;
}

} // namespace debugviewpp
} // namespace fusion
