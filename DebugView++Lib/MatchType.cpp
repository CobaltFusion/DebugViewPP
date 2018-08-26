// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <cassert>
#include "DebugView++Lib/MatchType.h"

namespace fusion {
namespace debugviewpp {

bool IsSpecialRegexCharacter(char c)
{
	switch (c)
	{
	case '^':
	case '$':
	case '\\':
	case '.':
	case '*':
	case '+':
	case '?':
	case '(':
	case ')':
	case '[':
	case ']':
	case '{':
	case '}':
	case '|': return true;
	default: return false;
	}
}

std::string MakeSimplePattern(const std::string& text)
{
	std::string pattern;
	for (auto c : text)
	{
		if (IsSpecialRegexCharacter(c))
			pattern += '\\';
		pattern += c;
	}
	return pattern;
}

std::string MakeWildcardPattern(const std::string& text)
{
	std::string pattern;
	for (auto c : text)
	{
		switch (c)
		{
		case '?': pattern += ".?"; break;
		case '*': pattern += ".*"; break;
		default: 
			if (IsSpecialRegexCharacter(c))
				pattern += '\\';
			pattern += c;
			break;
		}
	}
	return pattern;
}

std::string MakePattern(MatchType::type type, const std::string& text)
{
	switch (type)
	{
	case MatchType::Simple: return MakeSimplePattern(text);
	case MatchType::Wildcard: return MakeWildcardPattern(text);
	case MatchType::Regex:
	case MatchType::RegexGroups:
	case MatchType::RegexCase: return text;
	default: assert("Unexpected MatchType"); break;
	}
	return text;
}

int MatchTypeToInt(MatchType::type value)
{
#define MATCH_TYPE(f, id) case MatchType::f: return id;
	switch (value)
	{
	MATCH_TYPES()
	default: assert(!"Unexpected MatchType"); break;
	}
#undef MATCH_TYPE

	throw std::invalid_argument("bad MatchType!");
}

MatchType::type IntToMatchType(int value)
{
#define MATCH_TYPE(f, id) case id: return MatchType::f;
	switch (value)
	{
	MATCH_TYPES()
	default: assert(!"Unexpected MatchType"); break;
	}
#undef MATCH_TYPE

	throw std::invalid_argument("bad MatchType!");
}

std::string MatchTypeToString(MatchType::type value)
{
#define MATCH_TYPE(f, id) case MatchType::f: return #f;
	switch (value)
	{
	MATCH_TYPES()
	default: assert(!"Unexpected MatchType"); break;
	}
#undef MATCH_TYPE

	throw std::invalid_argument("bad MatchType!");
}

const wchar_t* EnumToWCharPtr(MatchType::type value)
{
#define MATCH_TYPE(f, id) case MatchType::f: return L ## #f;
	switch (value)
	{
	MATCH_TYPES()
	default: assert(!"Unexpected MatchType"); break;
	}
#undef MATCH_TYPE

	throw std::invalid_argument("bad MatchType!");
}

MatchType::type StringToMatchType(const std::string& s)
{
#define MATCH_TYPE(f, id) if (s == #f) return MatchType::f;
	MATCH_TYPES()
#undef MATCH_TYPE

	throw std::invalid_argument("bad MatchType!");
}

} // namespace debugviewpp 
} // namespace fusion
