// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Win32/Win32Lib.h"
#include "Win32/Registry.h"

namespace fusion {
namespace Win32 {

std::wstring RegGetStringValue(HKEY hKey, const wchar_t* valueName)
{
    long length = 0;
    long rc = ::RegQueryValue(hKey, valueName, nullptr, &length);
    if (rc != ERROR_SUCCESS)
    {
        ThrowWin32Error(rc, "RegQueryValue");
    }

    std::vector<wchar_t> data(length);
    rc = ::RegQueryValue(hKey, valueName, data.data(), &length);
    if (rc != ERROR_SUCCESS)
    {
        ThrowWin32Error(rc, "RegQueryValue");
    }
    return data.data();
}

std::wstring RegGetStringValue(HKEY hKey, const wchar_t* valueName, const wchar_t* defaultValue)
{
    DWORD type;
    DWORD length = 0;
    long rc = ::RegQueryValueEx(hKey, valueName, nullptr, &type, nullptr, &length);
    if (rc != ERROR_SUCCESS || type != REG_SZ)
    {
        return defaultValue;
    }

    std::vector<wchar_t> data(length);
    rc = ::RegQueryValueEx(hKey, valueName, nullptr, &type, reinterpret_cast<BYTE*>(data.data()), &length);
    if (rc != ERROR_SUCCESS)
    {
        return defaultValue;
    }

    return data.data();
}

DWORD RegGetDWORDValue(HKEY hKey, const wchar_t* valueName)
{
    DWORD type;
    DWORD value;
    DWORD count = sizeof(value);
    long rc = RegQueryValueEx(hKey, valueName, nullptr, &type, reinterpret_cast<BYTE*>(&value), &count);
    if (rc != ERROR_SUCCESS)
    {
        ThrowWin32Error(rc, "RegQueryValueEx");
    }
    if (type != REG_DWORD)
    {
        throw std::runtime_error("Invalid registry key");
    }

    return value;
}

DWORD RegGetDWORDValue(HKEY hKey, const wchar_t* valueName, DWORD defaultValue)
{
    DWORD type;
    DWORD value;
    DWORD count = sizeof(value);
    long rc = RegQueryValueEx(hKey, valueName, nullptr, &type, reinterpret_cast<BYTE*>(&value), &count);
    if (rc == ERROR_SUCCESS && type == REG_DWORD)
    {
        return value;
    }
    return defaultValue;
}

} // namespace Win32
} // namespace fusion
