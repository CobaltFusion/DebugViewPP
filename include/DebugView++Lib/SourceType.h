// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include <regex>
#include <memory>

namespace fusion {
namespace debugviewpp {

class LogSource;

struct SourceType		// the SourceType is for UI indication
{
	enum type
	{
		System,
		Pipe,
		File,
		UDP_Socket,
		TCP_Socket,
		Debugview_Agent
	};
};

struct SourceInfo
{
	SourceInfo(const std::wstring& description, SourceType::type sourceType);
	SourceInfo(const std::wstring& description, SourceType::type sourceType, const std::wstring& address, int port);
	SourceInfo(const std::wstring& description, SourceType::type sourceType, const std::wstring& address, int port, const std::shared_ptr<LogSource>& pLogSource);

	bool enabled;
	std::wstring description;
	SourceType::type type;
	std::wstring address;
	int port;
	std::shared_ptr<LogSource> pLogSource;
	bool remove;
};

int SourceTypeToInt(SourceType::type value);

SourceType::type IntToSourceType(int value);

std::string SourceTypeToString(SourceType::type value);

SourceType::type StringToSourceType(const std::string& s);

} // namespace debugviewpp 
} // namespace fusion
