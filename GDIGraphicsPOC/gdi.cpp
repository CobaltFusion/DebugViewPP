// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "gdi.h"
#include <string>

namespace fusion {
namespace gdi {

BOOL DeviceContextEx::DrawTextOut(const std::wstring& str, int x, int y)
{
	return CDC::TextOut(x, y, str.c_str(), str.size());
}


BOOL DeviceContextEx::DrawPolygon(const std::vector<POINT>& points)
{
	return CDC::Polygon(points.data(), points.size());
}

void DeviceContextEx::DrawTimeline(const std::wstring& name, int x, int y, int width, COLORREF color)
{
	auto pen = CreatePen(PS_SOLID, 1, color);
	SelectPen(pen);
	DrawTextOut(name, x, y -15);
	auto textWidth = 150;
	Rectangle(x + textWidth, y, x + textWidth + width, y + 2);
}

void DeviceContextEx::DrawFlag(const std::wstring& /* tooltip */, int x, int y)
{
	MoveTo(x, y);
	LineTo(x, y - 20);
	LineTo(x + 7, y - 16);
	LineTo(x, y - 12);
}

void DeviceContextEx::DrawSolidFlag(const std::wstring& /* tooltip */, int x, int y)
{
	DrawPolygon({ { x, y - 20 },{ x + 7, y - 16 },{ x, y - 12 } });
	MoveTo(x, y);
	LineTo(x, y - 20);
}

void DeviceContextEx::DrawSolidFlag(const std::wstring& tooltip, int x, int y, COLORREF border, COLORREF fill)
{
	auto pen = CreatePen(PS_SOLID, 1, border);
	SelectPen(pen);
	auto b = ::CreateSolidBrush(fill);
	SelectBrush(b);
	DrawSolidFlag(tooltip, x, y);
}

void DeviceContextEx::DrawFlag(const std::wstring& tooltip, int x, int y, COLORREF color, bool solid)
{
	auto pen = CreatePen(PS_SOLID, 1, color);
	SelectPen(pen);
	if (solid)
		DrawSolidFlag(tooltip, x, y);
	else
		DrawFlag(tooltip, x, y);
}

} // namespace graphics
} // namespace fusion
