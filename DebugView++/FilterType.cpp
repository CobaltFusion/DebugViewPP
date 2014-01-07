// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <stdexcept>
#include "FilterType.h"

namespace fusion {
namespace debugviewpp {

int FilterTypeToInt(FilterType::type value)
{
	return value;
}

#define FILTER_TYPES \
	FILTER_TYPE(Include) \
	FILTER_TYPE(Exclude) \
	FILTER_TYPE(Highlight) \
	FILTER_TYPE(Token) \
	FILTER_TYPE(Track) \
	FILTER_TYPE(Stop) \
	FILTER_TYPE(Ignore)

FilterType::type IntToFilterType(int value)
{
#define FILTER_TYPE(f) case FilterType::f: return FilterType::f;
	switch (value)
	{
	FILTER_TYPES
	default: assert(!"Unexpected FilterType"); break;
	}
#undef FILTER_TYPE

	throw std::invalid_argument("bad FilterType!");
}

std::string FilterTypeToString(FilterType::type value)
{
#define FILTER_TYPE(f) case FilterType::f: return #f;
	switch (value)
	{
	FILTER_TYPES
	default: assert(!"Unexpected FilterType"); break;
	}
#undef FILTER_TYPE

	throw std::invalid_argument("bad FilterType!");
}

const wchar_t* EnumToWCharPtr(FilterType::type value)
{
#define FILTER_TYPE(f) case FilterType::f: return L ## #f;
	switch (value)
	{
	FILTER_TYPES
	default: assert(!"Unexpected FilterType"); break;
	}
#undef FILTER_TYPE

	throw std::invalid_argument("bad FilterType!");
}

FilterType::type StringToFilterType(const std::string& s)
{
#define FILTER_TYPE(f) if (s == #f) return FilterType::f;
	FILTER_TYPES
#undef FILTER_TYPE

	throw std::invalid_argument("bad FilterType!");
}

} // namespace debugviewpp 
} // namespace fusion
