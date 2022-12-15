// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "Win32/Win32Lib.h"
#include "Win32/Com.h"
#include <Ole2.h>

namespace fusion {
namespace Win32 {

ComInitialization::ComInitialization()
{
    // OleInitialize/OleUninitialize is used instead of CoInitializeEx/CoUninitialize because it is required for use of IDropTarget's Drag and Drop functions
    HRESULT hr = OleInitialize(nullptr);
    if (FAILED(hr))
    {
        throw Win32Error(hr, "OleInitialize");
    }
}

ComInitialization::~ComInitialization()
{
    OleUninitialize();
}

} // namespace Win32
} // namespace fusion
