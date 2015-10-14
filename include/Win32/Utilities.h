// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include <cmath>

namespace fusion {

std::wstring LoadString(int id);

std::wstring GetExceptionMessage();

template <typename T>
T floor_to(double value)
{
	return static_cast<T>(std::floor(value));
}

} // namespace fusion
