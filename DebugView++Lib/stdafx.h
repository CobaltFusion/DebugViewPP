// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "targetver.h"

#ifdef _DEBUG
#include <string>
#include <vector>
#include <sstream>
#include <regex>
#include <vector>
#include <filesystem>
#include <thread>

#include <system_error>
#include <boost/date_time/local_time/local_time.hpp> 
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/core/noncopyable.hpp>
#endif

#include "atlbase.h"
#include "windows.h"
