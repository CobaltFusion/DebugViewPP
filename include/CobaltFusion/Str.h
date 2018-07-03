// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include "Win32/Win32Lib.h"

namespace fusion {

class Str
{
public:
	explicit Str(std::string s) :
		m_str(std::move(s))
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

	const char* c_str() const
	{
		return m_str.c_str();
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
	explicit WStr(const std::string& s) :
		m_str(Win32::MultiByteToWideChar(s))
	{
	}

	explicit WStr(const std::wstring& s) :
		m_str(s)
	{
	}

	std::wstring str() const
	{
		return m_str;
	}

	const wchar_t* c_str() const
	{
		return m_str.c_str();
	}

    operator std::wstring() const
	{
		return m_str;
	}

    operator const wchar_t*() const
	{
		return m_str.c_str();
	}

private:
	std::wstring m_str;
};

} // namespace fusion
