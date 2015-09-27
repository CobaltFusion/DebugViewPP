// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <cassert>
#include <stdexcept>
#include "DebugView++Lib/FilterType.h"

namespace fusion {
namespace debugviewpp {

// Do not change existing values, these number are stored in files to represent the FilterType enum
// Extend with new FilterType values at the end.
// See FilterTypeToInt() and IntToFilterType() for conversion to/from FilterType enum
namespace FilterFileIds
{
	const int Include = 0;
	const int Exclude = 1;
	const int Highlight = 2;
	const int Token = 3;
	const int Stop = 4;
	const int Track = 5;
	const int Once = 6;
	const int Clear = 7;
	const int Beep = 8;
	const int MatchColor = 9;
}

int FilterTypeToInt(FilterType::type value)
{
#define FILTER_TYPE(f) case FilterType::f: return FilterFileIds::f;
	switch (value)
	{
	FILTER_TYPES()
	default: assert(!"Unexpected FilterType"); break;
	}
#undef FILTER_TYPE

	throw std::invalid_argument("bad FilterType!");
}

FilterType::type IntToFilterType(int value)
{
#define FILTER_TYPE(f) case FilterFileIds::f: return FilterType::f;
	switch (value)
	{
	FILTER_TYPES()
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
	FILTER_TYPES()
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
	FILTER_TYPES()
	default: assert(!"Unexpected FilterType"); break;
	}
#undef FILTER_TYPE

	throw std::invalid_argument("bad FilterType!");
}

FilterType::type StringToFilterType(const std::string& s)
{
#define FILTER_TYPE(f) if (s == #f) return FilterType::f;
	FILTER_TYPES()
#undef FILTER_TYPE

	throw std::invalid_argument("bad FilterType!");
}

} // namespace debugviewpp 
} // namespace fusion
