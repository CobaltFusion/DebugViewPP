// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>

namespace fusion {
namespace debugviewpp {

#define MATCH_TYPES() \
	MATCH_TYPE(Simple) \
	MATCH_TYPE(Wildcard) \
	MATCH_TYPE(Regex) \
	MATCH_TYPE(RegexGroups)

struct MatchType
{
#define MATCH_TYPE(m) m,
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
