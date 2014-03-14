// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include <regex>
#include "MatchType.h"

namespace fusion {
namespace debugviewpp {

struct SourceType
{
	enum type
	{
		System,
		Pipe,
		File
	};
};

int SourceTypeToInt(SourceType::type value);

SourceType::type IntToSourceType(int value);

std::string SourceTypeToString(SourceType::type value);

const wchar_t* EnumToWCharPtr(SourceType::type value);

SourceType::type StringToSourceType(const std::string& s);

} // namespace debugviewpp 
} // namespace fusion
