// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <stdexcept>
#include <cassert>
#include "DebugView++Lib/SourceType.h"

namespace fusion {
namespace debugviewpp {

SourceInfo::SourceInfo(const std::wstring& description, SourceType::type type) :
	enabled(false),
	description(description),
	type(type),
	port(0)
{
}

SourceInfo::SourceInfo(const std::wstring& description, SourceType::type type, const std::wstring& address, int port) :
	enabled(false),
	description(description),
	type(type),
	address(address),
	port(port)
{
}

int SourceTypeToInt(SourceType::type value)
{
#define SOURCE_TYPE(en, id, name) case SourceType::en: return id;
	switch (value)
	{
	SOURCE_TYPES()
	default: assert(!"Unexpected SourceType"); break;
	}
#undef SOURCE_TYPE

	throw std::invalid_argument("bad SourceType!");
}

SourceType::type IntToSourceType(int value)
{
#define SOURCE_TYPE(en, id, name) case id: return SourceType::en;
	switch (value)
	{
	SOURCE_TYPES()
	default: assert(!"Unexpected SourceType"); break;
	}
#undef SOURCE_TYPE

	throw std::invalid_argument("bad SourceType!");
}

std::string SourceTypeToString(SourceType::type value)
{
#define SOURCE_TYPE(en, id, name) case SourceType::en: return name;
	switch (value)
	{
	SOURCE_TYPES()
	default: assert(!"Unexpected SourceType"); break;
	}
#undef SOURCE_TYPE

	throw std::invalid_argument("bad SourceType!");
}

SourceType::type StringToSourceType(const std::string& s)
{
#define SOURCE_TYPE(en, id, name) if (s == name) return SourceType::en;
	SOURCE_TYPES()
#undef SOURCE_TYPE

	throw std::invalid_argument("bad SourceType!");
}

} // namespace debugviewpp 
} // namespace fusion
