// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "DebugView++Lib/TimelineDC.h"
#include <string>
#include <cassert>

namespace fusion {
namespace gdi {

// todo: find out how this works (DECLARE_HANDLE(HDC);
// ::GetClientRect(m_hDC, &rect); is a compiler-time error, which is pretty cool
RECT TimelineDC::GetClientArea() const
{
    RECT rect;
    GetClipBox(&rect);
    return rect;
}

void TimelineDC::DrawTextOut(const std::wstring& str, int x, int y)
{
    TextOut(x, y, str.c_str(), static_cast<int>(str.size()));
}

void TimelineDC::DrawPolygon(const std::vector<POINT>& points)
{
    Polygon(points.data(), static_cast<int>(points.size()));
}

void TimelineDC::DrawTimeline(const std::wstring& name, Pixel x, Pixel y, int width, COLORREF color)
{
    CPen pen(CreatePen(PS_SOLID, 1, color));
    SelectPen(pen);
    DrawTextOut(name, x + 15, y - 15);
    Rectangle(x + s_leftTextAreaBorder, y, x + s_leftTextAreaBorder + width, y + 2);
}

void TimelineDC::DrawFlag(const std::wstring& /* tooltip */, int x, int y)
{
    // tooltip not implemented, see https://www.codeproject.com/Articles/5411/CToolTipDialog-class-a-simple-WTL-class-to-enable
    // and https://docs.microsoft.com/en-gb/windows/desktop/Controls/using-tooltip-contro
    // and https://docs.microsoft.com/en-us/windows/desktop/controls/implement-tracking-tooltips
    MoveTo(x, y);
    LineTo(x, y - 20);
    LineTo(x + 7, y - 16);
    LineTo(x, y - 12);
}

void TimelineDC::DrawSolidFlag(const std::wstring& /* tooltip */, int x, int y)
{
    DrawPolygon({{x, y - 20}, {x + 7, y - 16}, {x, y - 12}});
    MoveTo(x, y);
    LineTo(x, y - 20);
}

void TimelineDC::DrawSolidFlag(const std::wstring& tooltip, int x, int y, COLORREF border, COLORREF fill)
{
    CPen pen(CreatePen(PS_SOLID, 1, border));
    SelectPen(pen);
    CBrush b(CreateSolidBrush(fill));
    SelectBrush(b);
    DrawSolidFlag(tooltip, x, y);
}

void TimelineDC::DrawFlag(const std::wstring& tooltip, int x, int y, COLORREF color, bool solid)
{
    CPen pen(CreatePen(PS_SOLID, 1, color));
    SelectPen(pen);
    if (solid)
    {
        DrawSolidFlag(tooltip, x, y);
    }
    else
    {
        DrawFlag(tooltip, x, y);
    }
}

} // namespace gdi
} // namespace fusion
