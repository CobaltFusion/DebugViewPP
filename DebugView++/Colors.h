// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

namespace fusion {
namespace debugviewpp {

namespace Colors {

extern const COLORREF BackGround;
extern const COLORREF Text;
extern const COLORREF Highlight;
extern const COLORREF Selection;
extern const COLORREF ItemHighlight;
extern const COLORREF ItemHighlightText;

} // namespace Colors

COLORREF GetRandomBackColor();
COLORREF GetRandomTextColor();
COLORREF GetRandomProcessColor();

} // namespace debugviewpp 
} // namespace fusion
