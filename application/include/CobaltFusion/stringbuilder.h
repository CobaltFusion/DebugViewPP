// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <sstream>
#include "CobaltFusion/Str.h"

namespace fusion {

class stringbuilder
{
public:
    template <typename T>
    stringbuilder& operator<<(const T& t)
    {
        m_ss << t;
        return *this;
    }

    stringbuilder& operator<<(std::wstring_view str)
    {
        m_ss << Str(str); // replace with string_cast<>
        return *this;
    }

    stringbuilder& operator<<(const std::wstring& str)
    {
        m_ss << Str(str); // replace with string_cast<>
        return *this;
    }

    operator std::string() const
    {
        return m_ss.str();
    }

private:
    std::stringstream m_ss;
};

class wstringbuilder
{
public:
    template <typename T>
    wstringbuilder& operator<<(const T& t)
    {
        m_ss << t;
        return *this;
    }

    wstringbuilder& operator<<(const std::string& str)
    {
        m_ss << WStr(str);
        return *this;
    }

    operator std::wstring() const
    {
        return m_ss.str();
    }

private:
    std::wstringstream m_ss;
};


} // namespace fusion
