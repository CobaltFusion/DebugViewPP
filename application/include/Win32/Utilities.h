// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#pragma once

#include <string>

namespace fusion {
namespace win32 {

std::wstring LoadWString(int id);

static const int False = 0;
static const int True = 1;

} // namespace win32
} // namespace fusion
