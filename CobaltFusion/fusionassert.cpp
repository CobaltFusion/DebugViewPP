// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include "CobaltFusion/fusionassert.h"
#include "CobaltFusion/stringbuilder.h"
#include "Win32/Win32Lib.h"

#include <boost/algorithm/string.hpp>

#include <string>

namespace fusion {

void errormessage(const std::string& message, const std::string& caption)
{
    MessageBoxA(nullptr, message.c_str(), caption.c_str(), MB_OK | MB_ICONERROR);
}

void errormessage(const std::wstring& message, const std::wstring& caption)
{
    MessageBoxW(nullptr, message.c_str(), caption.c_str(), MB_OK | MB_ICONERROR);
}

void assertmessage(const std::string& assertion, const std::string& message, const char* location)
{
    errormessage(stringbuilder() << "Assertion '" << assertion << "' failed (" << message << ") at " << location, "Exception occurred");
}

void exceptionmessage(const char* what, const char* location)
{
    auto trimmed = boost::trim_copy_if(std::string(what), boost::is_any_of("\r\n\t"));
    errormessage(stringbuilder() << "Exception '" << trimmed << "' occured at " << location, "Exception occurred");
}

void exceptionmessage(const std::exception& ex, const char* location)
{
    exceptionmessage(ex.what(), location);
}

} // namespace fusion
