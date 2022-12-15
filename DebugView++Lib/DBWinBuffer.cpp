// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "Win32/Win32Lib.h"
#include "DebugView++Lib/DBWinBuffer.h"

namespace fusion {
namespace debugviewpp {

// this method is used to prevent acquiring the global DBWIN_BUFFER on XP, which will otherwise popup a MessageBox with a tip to 'Run As Administator'
// however, as there are no 'global' messages there, this does not apply to WindowsXP
bool IsWindowsVistaOrGreater()
{
    // consider using ::AtlIsOldWindows? needs to be tested on XP
    OSVERSIONINFO osvi = {};
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
// http://stackoverflow.com/questions/27246562/how-to-get-the-os-version-in-win8-1-as-getversion-getversionex-are-deprecated
// it looks like we can safely suppress this warning
#pragma warning(suppress : 4996)
    GetVersionEx(&osvi);
    return (osvi.dwMajorVersion > 5);
}

bool IsDBWinViewerActive()
{
    Win32::Handle hMap(::OpenFileMapping(FILE_MAP_READ, 0, L"DBWIN_BUFFER"));
    return hMap != nullptr;
}

bool HasGlobalDBWinReaderRights()
{
    Win32::Handle hMap(::CreateFileMapping(nullptr, nullptr, PAGE_READWRITE, 0, sizeof(DbWinBuffer), L"Global\\DBWIN_BUFFER"));
    return hMap != nullptr;
}

} // namespace debugviewpp
} // namespace fusion
