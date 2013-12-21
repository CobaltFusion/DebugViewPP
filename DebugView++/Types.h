// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

namespace fusion {
 
struct FilterType
{
	enum type
	{
		Include,
		Exclude,
		Highlight,
		Token,
		Stop,
		Track
	};
};

int FilterTypeToInt(FilterType::type value);

FilterType::type IntToFilterType(int value);

} // namespace fusion
