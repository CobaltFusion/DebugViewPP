// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "stdafx.h"
#include <string>
#include "Win32/Win32Lib.h"
#include "CobaltFusion/stringbuilder.h"
#include <boost/algorithm/string.hpp>

namespace fusion {

	void errorMessage(const std::string& caption, const std::string& message)
	{
		::MessageBoxA(0, message.c_str(), caption.c_str(), MB_ICONEXCLAMATION);
	}

	void assertmessage(const std::string& assertion, const std::string& message, const char * location)
	{
		errorMessage("Assertion failed", stringbuilder() << "Assertion '" << assertion << "' failed (" << message << ") at " << location);
	}

	void exceptionmessage(const char* what, const char * location)
	{
		auto trimmed = boost::trim_copy_if(std::string(what), boost::is_any_of("\r\n\t"));
		errorMessage("Exception occurred",  stringbuilder() << "Exception '" << trimmed << "' occured at " << location);
	}

} // namespace fusion
