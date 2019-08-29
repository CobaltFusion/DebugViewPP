// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include "Win32/Win32Lib.h"

namespace fusion {
namespace debugviewpp {

class DBWinWriter
{
public:
    DBWinWriter();

    void Write(DWORD pid, const std::string& message);

private:
    Win32::Handle m_hBuffer;
    Win32::Handle m_dbWinBufferReady;
    Win32::Handle m_dbWinDataReady;
    Win32::MappedViewOfFile m_dbWinView;
};

} // namespace debugviewpp
} // namespace fusion
