// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "windows.h"

namespace fusion {
namespace debugviewpp {

namespace Colors {

extern const COLORREF Auto;

// The colors of the theme (the system colors, or the colors of the dark theme), these change when the theme is set,
// so always use them where is is drawn and do not keep a copy.
extern COLORREF BackGround;
extern COLORREF Text;
extern COLORREF Highlight;
extern COLORREF Selection;
extern COLORREF ItemHighlight;
extern COLORREF ItemHighlightText;

void SetDarkTheme(bool dark);
bool IsDarkTheme();

// The color a color that was picked for a light background (random process colors, colors for matches) should have in the theme,
// the same color for the light theme, a darker color for the dark theme.
COLORREF Adapt(COLORREF color);

// The text color to use on the background color, in the light theme this is the normal text color (the background colors are
// light), in the dark theme it is black on a light and white on a dark background.
COLORREF TextOn(COLORREF background);

} // namespace Colors

COLORREF GetRandomBackColor();
COLORREF GetRandomTextColor();
COLORREF GetRandomProcessColor();

} // namespace debugviewpp
} // namespace fusion
