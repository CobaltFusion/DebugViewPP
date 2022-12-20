// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

namespace fusion {
namespace Win32 {

WINDOWPLACEMENT GetWindowPlacement(HWND hwnd);
POINT GetMessagePos();
POINT GetCursorPos();

} // namespace Win32
} // namespace fusion
