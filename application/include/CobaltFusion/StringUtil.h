// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

namespace fusion {

// String helpers: case insensitive compare and search, trimming.
//
// - 'Case insensitive' is ASCII only: A-Z are the same as a-z, every other character (also non-ASCII text in UTF-8) is compared as it is.
//   That is what the default locale does, and it is cheap, which matters for the search that is done for every line.
// - Arguments can be std::string, std::wstring, string_view and string literals, a narrow and a wide string can be mixed
//   (comparing an ASCII text like ".txt" with a wide file name).
// - Nothing allocates, except ToLower and TrimCopy that return a new string.

namespace detail {

template <typename S>
auto AsView(const S& s) -> std::basic_string_view<typename S::value_type>
{
    return std::basic_string_view<typename S::value_type>(s);
}

// a pointer to a 0 terminated string (not an array: that is a string literal, which is the overload below)
template <typename P>
auto AsView(const P& s) -> std::enable_if_t<std::is_pointer_v<P>, std::basic_string_view<std::remove_cv_t<std::remove_pointer_t<P>>>>
{
    return std::basic_string_view<std::remove_cv_t<std::remove_pointer_t<P>>>(s);
}

template <typename C, size_t N>
std::basic_string_view<C> AsView(const C (&s)[N])
{
    return std::basic_string_view<C>(s, N - 1); // without the terminating 0
}

template <typename C>
constexpr C ToLowerAscii(C c)
{
    return (c >= C('A') && c <= C('Z')) ? static_cast<C>(c + (C('a') - C('A'))) : c;
}

template <typename C1, typename C2>
constexpr bool SameIgnoringCase(C1 a, C2 b)
{
    return ToLowerAscii(a) == ToLowerAscii(b);
}

template <typename C1, typename C2>
bool EqualIgnoringCase(std::basic_string_view<C1> a, std::basic_string_view<C2> b)
{
    if (a.size() != b.size())
    {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (!SameIgnoringCase(a[i], b[i]))
        {
            return false;
        }
    }
    return true;
}

} // namespace detail

template <typename A, typename B>
bool IEquals(const A& a, const B& b)
{
    return detail::EqualIgnoringCase(detail::AsView(a), detail::AsView(b));
}

template <typename A, typename B>
bool IStartsWith(const A& text, const B& prefix)
{
    const auto t = detail::AsView(text);
    const auto p = detail::AsView(prefix);
    return t.size() >= p.size() && detail::EqualIgnoringCase(t.substr(0, p.size()), p);
}

template <typename A, typename B>
bool IEndsWith(const A& text, const B& suffix)
{
    const auto t = detail::AsView(text);
    const auto s = detail::AsView(suffix);
    return t.size() >= s.size() && detail::EqualIgnoringCase(t.substr(t.size() - s.size()), s);
}

template <typename A, typename B>
bool EndsWith(const A& text, const B& suffix)
{
    const auto t = detail::AsView(text);
    const auto s = detail::AsView(suffix);
    if (t.size() < s.size())
    {
        return false;
    }
    const auto end = t.substr(t.size() - s.size());
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (end[i] != s[i])
        {
            return false;
        }
    }
    return true;
}

// The position of the first occurrence of the pattern at or after 'pos', or std::string_view::npos. Like std::string::find,
// an empty pattern is found at 'pos' (when it is not after the end).
template <typename A, typename B>
size_t IFind(const A& text, const B& pattern, size_t pos = 0)
{
    const auto t = detail::AsView(text);
    const auto p = detail::AsView(pattern);
    if (pos > t.size() || t.size() - pos < p.size())
    {
        return std::string_view::npos;
    }
    if (p.empty())
    {
        return pos;
    }

    const auto first = detail::ToLowerAscii(p[0]);
    const size_t last = t.size() - p.size();
    for (size_t i = pos; i <= last; ++i)
    {
        if (detail::ToLowerAscii(t[i]) != first)
        {
            continue;
        }

        size_t j = 1;
        while (j < p.size() && detail::SameIgnoringCase(t[i + j], p[j]))
        {
            ++j;
        }
        if (j == p.size())
        {
            return i;
        }
    }
    return std::string_view::npos;
}

// true if the pattern is in the text, an empty pattern is in every text
template <typename A, typename B>
bool IContains(const A& text, const B& pattern)
{
    return IFind(text, pattern) != std::string_view::npos;
}

// a copy with the ASCII letters in lower case
template <typename S>
S ToLower(S s)
{
    for (auto& c : s)
    {
        c = detail::ToLowerAscii(c);
    }
    return s;
}

// Removes the characters that are in 'chars' from the end (TrimRight) or from both ends (TrimCopy) of a string.
template <typename S, typename Chars>
void TrimRight(S& s, const Chars& chars)
{
    const auto set = detail::AsView(chars);
    size_t end = s.size();
    while (end > 0 && set.find(s[end - 1]) != decltype(set)::npos)
    {
        --end;
    }
    s.resize(end);
}

template <typename S, typename Chars>
S TrimCopy(const S& s, const Chars& chars)
{
    const auto set = detail::AsView(chars);
    size_t begin = 0;
    size_t end = s.size();
    while (begin < end && set.find(s[begin]) != decltype(set)::npos)
    {
        ++begin;
    }
    while (end > begin && set.find(s[end - 1]) != decltype(set)::npos)
    {
        --end;
    }
    return S(s, begin, end - begin);
}

} // namespace fusion
