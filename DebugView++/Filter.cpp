// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Filter.h"

namespace fusion {
namespace debugviewpp {

Filter::Filter() :
	matchType(MatchType::Simple),
	filterType(FilterType::Include),
	bgColor(RGB(255, 255, 255)),
	fgColor(RGB(  0,   0,   0)),
	enable(true)
{
}

Filter::Filter(const std::string& text, MatchType::type matchType, FilterType::type filterType, COLORREF bgColor, COLORREF fgColor, bool enable, int matchCount) :
	text(text), re(MakePattern(matchType, text), std::regex_constants::icase | std::regex_constants::optimize), matchType(matchType), filterType(filterType), bgColor(bgColor), fgColor(fgColor), enable(enable), matchCount(matchCount)
{
}

} // namespace debugviewpp 
} // namespace fusion
