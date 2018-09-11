// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#define NOMINMAX

// Change these values to use different versions
#define WINVER		    0x0600
#define _WIN32_WINNT    0x0600
#define _WIN32_IE	    0x0603
#define _RICHEDIT_VER	0x0100

#define _WTL_NO_CSTRING
#define _CRT_NON_CONFORMING_SWPRINTFS
#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <algorithm>
using std::min;
using std::max;

#include <boost/asio.hpp> // must be included _before_ windows.h
#include "windows.h"
#include "ShellApi.h"

#pragma warning(push)
#pragma warning(disable : 4091) // 'typedef ': ignored on left of 'tagGPFIDL_FLAGS' when no variable is declared
#pragma warning(disable : 4838) // conversion from 'int' to 'UINT' requires a narrowing conversion
#pragma warning(disable : 4838) // conversion from 'int' to 'UINT' requires a narrowing conversion
#pragma warning(disable : 4458) // declaration of 'pButtons' hides class member

#define _ATL_NO_DEBUG_CRT

#include <atlbase.h>
#include <atlapp.h>

#undef _CRT_NON_CONFORMING_SWPRINTFS
#undef _CRT_SECURE_NO_WARNINGS

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
#pragma warning(pop)

#undef _CRT_NON_CONFORMING_SWPRINTFS


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

#ifdef _DEBUG
#include <string>
#include <vector>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#endif
