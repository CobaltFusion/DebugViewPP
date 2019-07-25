// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "windowsx.h"
#include "Win32/Win32Lib.h"
#include "Win32/Window.h"

namespace fusion {
namespace Win32 {

WINDOWPLACEMENT GetWindowPlacement(HWND hwnd)
{
	WINDOWPLACEMENT placement;
	placement.length = sizeof(placement);
	if (::GetWindowPlacement(hwnd, &placement) == 0)
	{
		ThrowLastError("GetWindowPlacement");
	}
	return placement;
}

POINT GetMessagePos()
{
	DWORD pos = ::GetMessagePos();
	POINT pt = {GET_X_LPARAM(pos), GET_Y_LPARAM(pos)};
	return pt;
}

POINT GetCursorPos()
{
	POINT pos;
	if (GetCursorPos(&pos) == 0)
	{
		ThrowLastError("GetCursorPos");
	}
	return pos;
}

} // namespace Win32
} // namespace fusion
