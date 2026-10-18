// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace fusion {
namespace debugviewpp {

// The regular expressions of the filters, PCRE2 (https://www.pcre.org/current/doc/html/pcre2syntax.html) with the JIT.
// Compared to the ECMAScript syntax of std::regex that was used before, this adds lookbehind ((?<=...), (?<!...)), possessive
// quantifiers, atomic groups, named groups, \K and many other things, and it is much faster.
//
// - The text is bytes (UTF-8 is not interpreted): '.' matches one byte, and case insensitive means ASCII only, like it was.
// - '$' matches only at the end of the text (not before a final newline), like in ECMAScript.
// - A match that takes more than a million steps (a pattern that backtracks without end) counts as 'no match', and does not hang the program.

class RegexError : public std::runtime_error
{
public:
    RegexError(const std::string& message, size_t offset);

    size_t offset() const; // where in the pattern the problem is

private:
    size_t m_offset;
};

// The position and length of the whole match (group 0) and of every capture group of the pattern in the text that was searched.
// A group that did not take part in the match has the position npos.
struct RegexMatch
{
    std::string_view text;
    std::vector<std::pair<size_t, size_t>> groups;

    size_t size() const;                // the number of groups, the whole match included
    bool Matched(size_t group) const;
    std::string_view Group(size_t group = 0) const; // empty if the group did not take part
    size_t Position(size_t group = 0) const;
    size_t Length(size_t group = 0) const;
    size_t End() const;                 // the end of the whole match
};

// A compiled pattern. It is cheap to copy (the compiled code is shared) and can be used from several threads at the same time.
// A default constructed Regex is the empty pattern: it matches everything.
class Regex
{
public:
    Regex();
    Regex(const std::string& pattern, bool ignoreCase); // throws RegexError when the pattern is not valid
    ~Regex();

    bool Search(std::string_view text) const;

    // Searches from 'offset' in the text. The text before the offset is still there for lookbehind, \b and ^ (unlike a search in a substring).
    bool Search(std::string_view text, RegexMatch& match, size_t offset = 0) const;

    // Calls fn(const RegexMatch&) for every match, from left to right, without overlap.
    template <typename Fn>
    void ForEachMatch(std::string_view text, Fn&& fn) const
    {
        RegexMatch match;
        size_t offset = 0;
        while (offset <= text.size() && Search(text, match, offset))
        {
            fn(static_cast<const RegexMatch&>(match));
            offset = match.Length() == 0 ? match.End() + 1 : match.End(); // an empty match must not be found again
        }
    }

private:
    struct Code;
    std::shared_ptr<const Code> m_code;
};

} // namespace debugviewpp
} // namespace fusion
