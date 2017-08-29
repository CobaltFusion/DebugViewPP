// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <cstdlib>
#include "DebugView++Lib/Colors.h"
#include "CobaltFusion/Math.h"

namespace fusion {
namespace debugviewpp {

namespace Colors {

extern const COLORREF Auto = 0x80808080;
extern const COLORREF BackGround = GetSysColor(COLOR_WINDOW);
extern const COLORREF Text = GetSysColor(COLOR_WINDOWTEXT);
extern const COLORREF Highlight = RGB(255, 255, 55);
extern const COLORREF Selection = RGB(128, 255, 255);
extern const COLORREF ItemHighlight = GetSysColor(COLOR_HIGHLIGHT);
extern const COLORREF ItemHighlightText = GetSysColor(COLOR_HIGHLIGHTTEXT);

} // namespace Colors

COLORREF HsvToRgb(double h, double s, double v)
{
	int hi = FloorTo<int>(h*6);
	double f = h*6 - hi;
	int vi = FloorTo<int>(256 * v);
	int pi = FloorTo<int>(256 * v * (1 - s));
	int qi = FloorTo<int>(256 * v * (1 - f*s));
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
	static const double ratio = (1 + std::sqrt(5.))/2 - 1;
	// use golden ratio
	static double h = static_cast<double>(std::rand()) / (RAND_MAX + 1);

	h += ratio;
	if (h >= 1)
		h = h - 1;
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
