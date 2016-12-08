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

	void errormessage(const std::string& message, const std::string& caption)
	{
		MessageBoxA(nullptr, message.c_str(), caption.c_str() , MB_OK | MB_ICONERROR);
	}

	void assertmessage(const std::string& assertion, const std::string& message, const char * location)
	{
		errormessage(stringbuilder() << "Assertion '" << assertion << "' failed (" << message << ") at " << location, "Exception occurred");
	}

	void exceptionmessage(const char* what, const char * location)
	{
		auto trimmed = boost::trim_copy_if(std::string(what), boost::is_any_of("\r\n\t"));
		errormessage(stringbuilder() << "Exception '" << trimmed << "' occured at " << location, "Exception occurred");
	}

} // namespace fusion
