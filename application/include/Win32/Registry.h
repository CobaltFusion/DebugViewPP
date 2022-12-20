// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <windows.h>

namespace fusion {
namespace Win32 {

std::wstring RegGetStringValue(HKEY hKey, const wchar_t* valueName = nullptr);
std::wstring RegGetStringValue(HKEY hKey, const wchar_t* valueName, const wchar_t* defaultValue);

DWORD RegGetDWORDValue(HKEY hKey, const wchar_t* valueName = nullptr);
DWORD RegGetDWORDValue(HKEY hKey, const wchar_t* valueName, DWORD defaultValue);

} // namespace Win32
} // namespace fusion
