// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>

namespace fusion {
namespace debugviewpp {

// Do not change existing values, these number are stored in files to represent the FilterType enum
// Extend with new FilterType values at the end.
// See FilterTypeToInt() and IntToFilterType() for conversion to/from FilterType enum
#define FILTER_TYPES() \
	FILTER_TYPE(Include, 0) \
	FILTER_TYPE(Exclude, 1) \
	FILTER_TYPE(Highlight, 2) \
	FILTER_TYPE(Token, 3) \
	FILTER_TYPE(Stop, 4) \
	FILTER_TYPE(Track, 5) \
	FILTER_TYPE(Once, 6) \
	FILTER_TYPE(Clear, 7) \
	FILTER_TYPE(Beep, 8) \
	FILTER_TYPE(MatchColor, 9) \
	FILTER_TYPE(Bookmark, 10)

struct FilterType
{
#define FILTER_TYPE(f, id) f,
	enum type
	{
		FILTER_TYPES()
	};
#undef FILTER_TYPE
};

int FilterTypeToInt(FilterType::type value);

FilterType::type IntToFilterType(int value);

std::string FilterTypeToString(FilterType::type value);

const wchar_t* EnumToWCharPtr(FilterType::type value);

FilterType::type StringToFilterType(const std::string& s);

} // namespace debugviewpp 
} // namespace fusion
