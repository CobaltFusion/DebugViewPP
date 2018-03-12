// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <string>
#include <atlstr.h>
#include "Win32/Win32Lib.h"

namespace fusion {

std::wstring LoadString(int id)
{
	CString cs;
	if (!cs.LoadString(id))
		Win32::ThrowLastError("LoadString");
	return static_cast<const wchar_t*>(cs);
}

} // namespace fusion
