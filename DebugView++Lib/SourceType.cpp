// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <stdexcept>
#include "DebugView++Lib/SourceType.h"
#include "assert.h"

namespace fusion {
namespace debugviewpp {

int SourceTypeToInt(SourceType::type value)
{
	return value;
}

#define SOURCE_TYPES \
	SOURCE_TYPE(System) \
	SOURCE_TYPE(File) \
	SOURCE_TYPE(Pipe)

SourceType::type IntToSourceType(int value)
{
#define SOURCE_TYPE(f) case SourceType::f: return SourceType::f;
	switch (value)
	{
	SOURCE_TYPES
	default: assert(!"Unexpected SourceType"); break;
	}
#undef SOURCE_TYPE

	throw std::invalid_argument("bad SourceType!");
}

std::string SourceTypeToString(SourceType::type value)
{
#define SOURCE_TYPE(f) case SourceType::f: return #f;
	switch (value)
	{
	SOURCE_TYPES
	default: assert(!"Unexpected SourceType"); break;
	}
#undef SOURCE_TYPE

	throw std::invalid_argument("bad SourceType!");
}

const wchar_t* EnumToWCharPtr(SourceType::type value)
{
#define SOURCE_TYPE(f) case SourceType::f: return L ## #f;
	switch (value)
	{
	SOURCE_TYPES
	default: assert(!"Unexpected SourceType"); break;
	}
#undef SOURCE_TYPE

	throw std::invalid_argument("bad SourceType!");
}

SourceType::type StringToSourceType(const std::string& s)
{
#define SOURCE_TYPE(f) if (s == #f) return SourceType::f;
	SOURCE_TYPES
#undef SOURCE_TYPE

	throw std::invalid_argument("bad SourceType!");
}

} // namespace debugviewpp 
} // namespace fusion
