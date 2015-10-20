// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>

namespace fusion {
namespace debugviewpp {

class LogSource;

#define SOURCE_TYPES() \
	SOURCE_TYPE(System, 0, "System") \
	SOURCE_TYPE(Pipe, 1, "Pipe") \
	SOURCE_TYPE(File, 2, "File") \
	SOURCE_TYPE(Udp, 3, "UDP Socket") \
	SOURCE_TYPE(Tcp, 4, "TCP Socket") \
	SOURCE_TYPE(DebugViewAgent, 5, "DebugView Agent")

struct SourceType
{
#define SOURCE_TYPE(en, id, name) en,
	enum type
	{
		SOURCE_TYPES()
	};
#undef SOURCE_TYPE
};

struct SourceInfo
{
	SourceInfo(const std::wstring& description, SourceType::type sourceType);
	SourceInfo(const std::wstring& description, SourceType::type sourceType, const std::wstring& address, int port);

	bool enabled;
	std::wstring description;
	SourceType::type type;
	std::wstring address;
	int port;
};

int SourceTypeToInt(SourceType::type value);

SourceType::type IntToSourceType(int value);

std::string SourceTypeToString(SourceType::type value);

SourceType::type StringToSourceType(const std::string& s);

} // namespace debugviewpp 
} // namespace fusion
