// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>

namespace fusion {
namespace debugviewpp {

// Do not change existing values, these number are stored in files to represent the MatchType enum
// Extend with new MatchType values at the end.
// See MatchTypeToInt() and IntToMatchType() for conversion to/from MatchType enum
#define MATCH_TYPES() \
	MATCH_TYPE(Simple, 0) \
	MATCH_TYPE(Wildcard, 1) \
	MATCH_TYPE(Regex, 2) \
	MATCH_TYPE(RegexGroups, 3)

struct MatchType
{
#define MATCH_TYPE(m, id) m,
	enum type
	{
		MATCH_TYPES()
	};
#undef MATCH_TYPE
};

std::string MakePattern(MatchType::type type, const std::string& text);

int MatchTypeToInt(MatchType::type value);

MatchType::type IntToMatchType(int value);

std::string MatchTypeToString(MatchType::type value);

const wchar_t* EnumToWCharPtr(MatchType::type value);

MatchType::type StringToMatchType(const std::string& s);

} // namespace debugviewpp 
} // namespace fusion
