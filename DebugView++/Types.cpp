// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Types.h"

namespace fusion {

int FilterTypeToInt(FilterType::type value) 
{
	return value;
}

FilterType::type IntToFilterType(int value)
{
	switch (value)
	{
	case FilterType::Include: return FilterType::Include;
	case FilterType::Exclude: return FilterType::Exclude;
	case FilterType::Highlight: return FilterType::Highlight;
	case FilterType::Token: return FilterType::Token;
	case FilterType::Stop: return FilterType::Stop;
	case FilterType::Track: return FilterType::Track;
	default: assert(!"Unexpected FilterType"); break;
	}
	throw std::invalid_argument("bad FilterType!");
}

} // namespace fusion