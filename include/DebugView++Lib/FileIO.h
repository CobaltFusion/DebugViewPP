// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <iosfwd>
#include "DebugView++Lib/DBWinBuffer.h"

namespace fusion {
namespace debugviewpp {

std::istream& ReadLogFileMessage(std::istream& is, Line& line);
bool ReadLogFileMessage(const std::string& data, Line& line);

} // namespace debugviewpp 
} // namespace fusion
