// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#define WINVER		    0x0501
#define _WIN32_WINNT    0x0501

#pragma warning(disable : 4355) // 'this' : used in base member initializer list

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define NOMINMAX

#pragma warning(disable : 4503 4512 4996)	// boost warnings we cannot work around
#ifdef _DEBUG
#include <boost/noncopyable.hpp>
#include <boost/signals2.hpp>
#endif

#include <thread>
