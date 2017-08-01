// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Win32/Win32Lib.h"
#include "Win32/Com.h"

namespace fusion {
namespace Win32 {

ComInitialization::ComInitialization(CoInit init)
{
	HRESULT hr = OleInitialize(nullptr);
	if (FAILED(hr))
		throw Win32Error(hr, "OleInitialize");
}

ComInitialization::~ComInitialization()
{
	OleUninitialize();
}

} // namespace Win32
} // namespace fusion
