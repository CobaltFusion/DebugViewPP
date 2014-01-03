// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#define NOMINMAX

#include <stdint.h>
#include <algorithm>
using std::min;
using std::max;

// Change these values to use different versions
#define WINVER		0x0501
#define _WIN32_WINNT 0x0501
#define _WIN32_IE	0x0603
#define _RICHEDIT_VER	0x0100

#define _WTL_NO_CSTRING

#include <atlbase.h>

#pragma warning(push, 3)
#pragma warning(disable: 4996)
#include <atlapp.h>
#pragma warning(pop)

#include <atlwin.h>
#include <atlcrack.h>
#include <atlframe.h>
#include <atlsplit.h>
#include <atlctrls.h>
#include <atlctrlx.h>
#include <atlctrlw.h>
#include <atldlgs.h>
#include <atlmisc.h>
#include <atlstr.h>

extern CAppModule _Module;

#if defined _M_IX86
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#include <boost/utility.hpp>
#include <boost/signals2.hpp>
#include <boost/thread.hpp>

#include <string>
#include <vector>
#include <sstream>
#include <cmath>

#include <boost/utility.hpp>
#include <boost/system/system_error.hpp>
#include <boost/date_time/local_time/local_time.hpp> 
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

#include "libsnappy.h"
