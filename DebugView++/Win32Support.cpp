// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <vector>
#include "Win32Lib/Win32Lib.h"
#include "Win32Support.h"

namespace fusion {

ComInitialization::ComInitialization(CoInit init)
{
	HRESULT hr = CoInitializeEx(nullptr, init);
	if (FAILED(hr))
		throw Win32Error(hr, "CoInitializeEx");
}

ComInitialization::~ComInitialization()
{
	CoUninitialize();
}

WINDOWPLACEMENT GetWindowPlacement(HWND hwnd)
{
	WINDOWPLACEMENT placement;
	placement.length = sizeof(placement);
	if (!::GetWindowPlacement(hwnd, &placement))
		ThrowLastError("GetWindowPlacement");
	return placement;
}

POINT GetMessagePos()
{
	DWORD pos = ::GetMessagePos();
	POINT pt = { GET_X_LPARAM(pos), GET_Y_LPARAM(pos) };
	return pt;
}

POINT GetCursorPos()
{
	POINT pos;
	if (!GetCursorPos(&pos))
		ThrowLastError("GetCursorPos");
	return pos;
}

} // namespace fusion
