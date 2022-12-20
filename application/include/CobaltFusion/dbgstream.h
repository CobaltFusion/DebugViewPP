// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// OutputDebugString as an ostream:
// cdbg << "Hello " << name << std::endl;
// wcdbg << L"Hello " << wname << std::endl;
//
// cnull and wcnull are do-nothing stremas that support macro based logging on/off selection:
//
// #ifdef NDEBUG
// #    define CDBG cnull
// #    define WCDBG wcnull
// #else
// #    define CDBG cdbg
// #    define WCDBG wcdbg
// #endif


#ifndef DBGSTREAM_H
#define DBGSTREAM_H

#pragma once

#include <streambuf>
#include <ostream>
#include <string>
#include "windows.h"

namespace dbgstream {

template <class Elem, class Tr = std::char_traits<Elem>, class Alloc = std::allocator<Elem>>
class basic_debugbuf : public std::basic_streambuf<Elem, Tr>
{
    using _int_type = typename std::basic_streambuf<Elem, Tr>::int_type;
    using _traits_type = typename std::basic_streambuf<Elem, Tr>::traits_type;

protected:
    int sync() override
    {
        output(m_buf.c_str());
        m_buf.clear();
        return 0;
    }

    _int_type overflow(_int_type c) override
    {
        if (c == _traits_type::eof())
            return 0;

        m_buf += _traits_type::to_char_type(c);
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

template <class Elem, class Tr = std::char_traits<Elem>>
class basic_dbgstream : public std::basic_ostream<Elem, Tr>
{
public:
    basic_dbgstream() :
        std::basic_ostream<Elem, Tr>(&buf)
    {
    }

private:
    basic_debugbuf<Elem, Tr> buf;
};

template <class Elem, class Tr = std::char_traits<Elem>, class Alloc = std::allocator<Elem>>
class basic_nullbuf : public std::basic_streambuf<Elem, Tr>
{
};

template <class Elem, class Tr = std::char_traits<Elem>>
class basic_nullstream : public std::basic_ostream<Elem, Tr>
{
public:
    basic_nullstream() :
        std::basic_ostream<Elem, Tr>(&buf)
    {
    }

private:
    basic_nullbuf<Elem, Tr> buf;
};

using nullstream = basic_nullstream<char>;
using wnullstream = basic_nullstream<wchar_t>;

using dbgstream = basic_dbgstream<char>;
using wdbgstream = basic_dbgstream<wchar_t>;

} // namespace dbgstream

__declspec(selectany) dbgstream::nullstream cnull;
__declspec(selectany) dbgstream::wnullstream wcnull;

__declspec(selectany) dbgstream::dbgstream cdbg;
__declspec(selectany) dbgstream::wdbgstream wcdbg;

#endif // DBGSTREAM_H
