// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// #include "stdafx.h"
#include <cstdlib>
#include "DebugViewppLib/Colors.h"
#include "CobaltFusion/Math.h"

namespace fusion {
namespace debugviewpp {

namespace Colors {

extern const COLORREF Auto = 0x80808080;
COLORREF BackGround = GetSysColor(COLOR_WINDOW);
COLORREF Text = GetSysColor(COLOR_WINDOWTEXT);
COLORREF Highlight = RGB(255, 255, 55);
COLORREF Selection = RGB(128, 255, 255);
COLORREF ItemHighlight = GetSysColor(COLOR_HIGHLIGHT);
COLORREF ItemHighlightText = GetSysColor(COLOR_HIGHLIGHTTEXT);

static bool g_darkTheme = false;

void SetDarkTheme(bool dark)
{
    g_darkTheme = dark;
    if (dark)
    {
        BackGround = RGB(30, 30, 30);
        Text = RGB(220, 220, 220);
        Highlight = RGB(138, 110, 0);
        Selection = RGB(38, 79, 120);
        ItemHighlight = RGB(0, 90, 158);
        ItemHighlightText = RGB(255, 255, 255);
    }
    else
    {
        BackGround = GetSysColor(COLOR_WINDOW);
        Text = GetSysColor(COLOR_WINDOWTEXT);
        Highlight = RGB(255, 255, 55);
        Selection = RGB(128, 255, 255);
        ItemHighlight = GetSysColor(COLOR_HIGHLIGHT);
        ItemHighlightText = GetSysColor(COLOR_HIGHLIGHTTEXT);
    }
}

bool IsDarkTheme()
{
    return g_darkTheme;
}

COLORREF Adapt(COLORREF color)
{
    if (!g_darkTheme)
    {
        return color;
    }

    // the colors are light (a high value, a low saturation or a high saturation), make them dark but keep the hue
    constexpr int percentage = 40;
    return RGB(GetRValue(color) * percentage / 100, GetGValue(color) * percentage / 100, GetBValue(color) * percentage / 100);
}

COLORREF TextOn(COLORREF background)
{
    if (!g_darkTheme)
    {
        return Text;
    }

    // the perceived brightness of the background
    const int brightness = (GetRValue(background) * 299 + GetGValue(background) * 587 + GetBValue(background) * 114) / 1000;
    return brightness > 140 ? RGB(0, 0, 0) : RGB(240, 240, 240);
}

} // namespace Colors

COLORREF HsvToRgb(double h, double s, double v)
{
    int hi = FloorTo<int>(h * 6);
    double f = h * 6 - hi;
    int vi = FloorTo<int>(256 * v);
    int pi = FloorTo<int>(256 * v * (1 - s));
    int qi = FloorTo<int>(256 * v * (1 - f * s));
    int ti = FloorTo<int>(256 * v * (1 - (1 - f) * s));
    switch (hi)
    {
    case 0: return RGB(vi, ti, pi);
    case 1: return RGB(qi, vi, pi);
    case 2: return RGB(pi, vi, ti);
    case 3: return RGB(pi, qi, vi);
    case 4: return RGB(ti, pi, vi);
    case 5: return RGB(vi, pi, qi);
    default: break;
    }
    return 0;
}

COLORREF GetRandomColor(double s, double v)
{
    static bool randomize = (std::srand(GetTickCount()), true);
    static const double ratio = (1 + std::sqrt(5.)) / 2 - 1;
    // use golden ratio
    static double h = static_cast<double>(std::rand()) / (RAND_MAX + 1);

    h += ratio;
    if (h >= 1)
    {
        h = h - 1;
    }
    return HsvToRgb(h, s, v);
}

COLORREF GetRandomBackColor()
{
    return GetRandomColor(0.5, 0.95);
}

COLORREF GetRandomTextColor()
{
    return GetRandomColor(0.9, 0.7);
}

COLORREF GetRandomProcessColor()
{
    return GetRandomColor(0.20, 0.95);
}

} // namespace debugviewpp
} // namespace fusion
