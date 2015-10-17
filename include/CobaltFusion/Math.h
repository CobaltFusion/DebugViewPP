// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <cmath>

namespace fusion {

template <typename T>
T FloorTo(double value)
{
	return static_cast<T>(std::floor(value));
}

template <typename T>
T CeilTo(double value)
{
	return static_cast<T>(std::ceil(value));
}

template <typename T>
T RoundTo(double value)
{
	return static_cast<T>(std::round(value));
}

} // namespace fusion
