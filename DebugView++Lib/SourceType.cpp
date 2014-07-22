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

std::string RemoveSpaces(std::string value)
{
	boost::replace_all(value, " ", "_");
	return value;
}

std::string RestoreSpaces(std::string value)
{
	boost::replace_all(value, "_", " ");
	return value;
}

int SourceTypeToInt(SourceType::type value)
{
	return value;
}

#define SOURCE_TYPES \
	SOURCE_TYPE(System) \
	SOURCE_TYPE(File) \
	SOURCE_TYPE(Pipe) \
	SOURCE_TYPE(UDP_Socket) \
	SOURCE_TYPE(TCP_Socket) \
	SOURCE_TYPE(Debugview_Agent)

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
#define SOURCE_TYPE(f) case SourceType::f: return RestoreSpaces(#f);
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
#define SOURCE_TYPE(f) if (RemoveSpaces(s) == #f) return SourceType::f;
	SOURCE_TYPES
#undef SOURCE_TYPE

	throw std::invalid_argument("bad SourceType!");
}

} // namespace debugviewpp 
} // namespace fusion
