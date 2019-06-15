// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

// OutputDebugString as an ostream:
// cdbg << "Hello " << name << std::endl;
// wcdbg << L"Hello " << wname << std::endl;
// 
// cnull and wcnull are do-nothing stremas that support macro based logging on/off selection:
//
// #ifdef NDEBUG
// #	define CDBG cnull
// #	define WCDBG wcnull
// #else
// #	define CDBG cdbg
// #	define WCDBG wcdbg
// #endif

#ifndef DBGSTREAM_H
#define DBGSTREAM_H

#pragma once

#include <streambuf>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include "windows.h"

namespace dbgstream {

template <class Elem, class Tr = std::char_traits<Elem>, class Alloc = std::allocator<Elem> >
class basic_debugbuf : public std::basic_streambuf<Elem, Tr>
{
protected:
    int sync() override
	{
		output(m_buf.c_str());
		m_buf.clear();
		return 0;
	}

	using int_type = std::basic_streambuf<Elem, Tr>::int_type;
	int_type overflow(int_type c) override
	{
		if (c == std::basic_streambuf<Elem, Tr>::traits_type::eof())
			return 0;

		m_buf += std::basic_streambuf<Elem, Tr>::traits_type::to_char_type(c);
		if (c == '\n')
			sync();
		return c;
	}

private:
	std::basic_string<Elem, Tr, Alloc> m_buf;

	static void output(const char* msg)
	{
		OutputDebugStringA(msg);
	}

	static void output(const wchar_t* msg)
	{
		OutputDebugStringW(msg);
	}
};

template <class Elem, class Tr = std::char_traits<Elem> >
class basic_dbgstream : public std::basic_ostream<Elem, Tr>
{
public:
	basic_dbgstream() : std::basic_ostream<Elem, Tr>(&buf)
	{
	}

private:
	basic_debugbuf<Elem, Tr> buf;
};

template <class Elem, class Tr = std::char_traits<Elem>, class Alloc = std::allocator<Elem> >
class basic_nullbuf : public std::basic_streambuf<Elem, Tr>
{
};

template <class Elem, class Tr = std::char_traits<Elem> >
class basic_nullstream : public std::basic_ostream<Elem, Tr>
{
public:
	basic_nullstream() : std::basic_ostream<Elem, Tr>(&buf)
	{
	}

private:
	basic_nullbuf<Elem, Tr> buf;
};

typedef basic_nullstream<char> nullstream;
typedef basic_nullstream<wchar_t> wnullstream;

typedef basic_dbgstream<char> dbgstream;
typedef basic_dbgstream<wchar_t> wdbgstream;

} // namespace dbgstream

__declspec(selectany) dbgstream::nullstream cnull;
__declspec(selectany) dbgstream::wnullstream wcnull;

__declspec(selectany) dbgstream::dbgstream cdbg;
__declspec(selectany) dbgstream::wdbgstream wcdbg;

#endif // DBGSTREAM_H
