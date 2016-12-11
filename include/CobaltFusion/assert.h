// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include "Win32/Win32Lib.h"
#include "CobaltFusion/stringbuilder.h"

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)

#ifdef NDEBUG
	#define FUSION_ASSERT(cond, msg) ((void)0)
#else
	#define FUSION_ASSERT(cond, msg) if (!(cond)) fusion::assertmessage(#cond, msg, __FILE__ ## ":" ## LINE_STRING);
#endif

#define FUSION_ASSERT_ALWAYS(cond, msg) if (!(cond)) fusion::assertmessage(#cond, msg, __FILE__ ## ":" ## LINE_STRING);
#define FUSION_REPORT_EXCEPTION(what) fusion::exceptionmessage(what, __FILE__ ## ":" ## LINE_STRING);

namespace fusion {

	void errormessage(const std::string& message, const std::string& caption = "DebugView++ Error");
	void assertmessage(const std::string& assertion, const std::string& message, const char * location);
	void exceptionmessage(const char* what, const char * location);

} // namespace fusion
