// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include <string_view>
#include "Win32/Win32Lib.h"

namespace fusion {

class Str
{
public:
	explicit Str(std::string_view s) :
		m_str(s)
	{
	}

	explicit Str(const char* s) :
		m_str(s)
	{
	}

	explicit Str(const std::wstring& s) :
		m_str(Win32::WideCharToMultiByte(s))
	{
	}

	explicit Str(const wchar_t* s) :
		m_str(Win32::WideCharToMultiByte(s))
	{
	}
	
	std::string str() const
	{
		return m_str;
	}

	operator std::string() const
	{
		return m_str;
	}

	operator const char*() const
	{
		return m_str.c_str();
	}

private:
	std::string m_str;
};

class WStr
{
public:
	// replace these with string_cast<>
	explicit WStr(std::string_view s) :
		m_str(Win32::MultiByteToWideChar(s))
	{
	}

	// usefull to convert a stringbuilder() into an wstring when the function needs a wchar_t*
	explicit WStr(const std::wstring& s) :
		m_str(s)
	{
	}

	// replace these with string_cast<>
	std::wstring str() const
	{
		return m_str;
	}

	// replace these with string_cast<>
	operator std::wstring() const
	{
		return m_str;
	}

	// nice to have a temporart WStr decay into a wchar_t* but also easy to use wrong like "const wchar_t* p = WStr(msg.text);" is very wrong.
	operator const wchar_t*() const
	{
		return m_str.c_str();
	}

private:
	std::wstring m_str;
};

} // namespace fusion
