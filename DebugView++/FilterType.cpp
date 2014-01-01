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

FilterType::type IntToFilterType(int value)
{
	switch (value)
	{
	case FilterType::Include: return FilterType::Include;
	case FilterType::Exclude: return FilterType::Exclude;
	case FilterType::Highlight: return FilterType::Highlight;
	case FilterType::Token: return FilterType::Token;
	case FilterType::Track: return FilterType::Track;
	case FilterType::Stop: return FilterType::Stop;
	default: assert(!"Unexpected FilterType"); break;
	}
	throw std::invalid_argument("bad FilterType!");
}

std::string FilterTypeToString(FilterType::type value)
{
	switch (value)
	{
	case FilterType::Include: return "Include";
	case FilterType::Exclude: return "Exclude";
	case FilterType::Highlight: return "Highlight";
	case FilterType::Token: return "Token";
	case FilterType::Track: return "Track";
	case FilterType::Stop: return "Stop";
	default: assert(!"Unexpected FilterType"); break;
	}
	throw std::invalid_argument("bad FilterType!");
}

FilterType::type StringToFilterType(const std::string& s)
{
#define FILTER_TYPE(f) if (boost::iequals(s, _T(#f))) return FilterType::f;
	FILTER_TYPE(Include);
	FILTER_TYPE(Exclude);
	FILTER_TYPE(Highlight);
	FILTER_TYPE(Token);
	FILTER_TYPE(Track);
	FILTER_TYPE(Stop);
#undef FILTER_TYPE

	return IntToFilterType(boost::lexical_cast<int>(s));
}

} // namespace debugviewpp 
} // namespace fusion
