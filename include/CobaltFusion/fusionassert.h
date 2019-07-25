// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>

// clang-format off
#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)
#define SOURCE_LOCATION __FILE__ ## ":" ## LINE_STRING

#ifdef NDEBUG
	#define FUSION_ASSERT(cond, msg) ((void)0)
#else
	#define FUSION_ASSERT(cond, msg) if (!(cond)) fusion::assertmessage(#cond, msg, SOURCE_LOCATION);
#endif

#define FUSION_ASSERT_ALWAYS(cond, msg) if (!(cond)) fusion::assertmessage(#cond, msg, SOURCE_LOCATION);
#define FUSION_REPORT_EXCEPTION(ex) fusion::exceptionmessage(ex, SOURCE_LOCATION);
// clang-format on

namespace fusion {

void errormessage(const std::string& message, const std::string& caption);
void errormessage(const std::wstring& message, const std::wstring& caption);
void assertmessage(const std::string& assertion, const std::string& message, const char* location);
void exceptionmessage(const char* what, const char* location);
void exceptionmessage(const std::exception& ex, const char* location);

} // namespace fusion
