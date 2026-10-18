// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "windows.h"

namespace fusion {
namespace debugviewpp {

struct DbWinBuffer
{
    DWORD processId;
    // Total size must be 4KB (processID + data)
    char data[4096 - sizeof(DWORD)];
};

static_assert(sizeof(DbWinBuffer) == 4096, "DBWIN_BUFFER size must be 4096");

bool IsWindowsVistaOrGreater();
bool IsDBWinViewerActive();
bool HasGlobalDBWinReaderRights();

// Services run in session 0, there the local and the Global\ object namespace are the same. A reader for the local namespace
// then already receives all messages, and creating a second one for Global\ fails, because DBWIN_BUFFER already exists.
bool IsSessionZero();

// true if, next to the reader for the local namespace, a reader for the Global\ namespace is possible and useful.
bool ShouldAddGlobalDBWinReader();

} // namespace debugviewpp
} // namespace fusion
