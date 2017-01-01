// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DebugView++Lib/CTimelineView.h"
#include <string>
#include <cassert>
#include "CobaltFusion/dbgstream.h"
#include "CobaltFusion/Str.h"
#include "CobaltFusion/stringbuilder.h"

namespace fusion {
namespace gdi {

Artifact::Artifact(int position, Artifact::Type type) :
	m_position(position),
	m_type(type),
	m_color(RGB(0, 0, 0)),
	m_fillcolor(RGB(255, 255, 255))
{
}

Artifact::Artifact(int position, Artifact::Type type, COLORREF color) :
	m_position(position),
	m_type(type),
	m_color(color),
	m_fillcolor(RGB(255, 255, 255))
{
}

Artifact::Artifact(int position, Artifact::Type type, COLORREF color, COLORREF fillcolor) :
	m_position(position),
	m_type(type),
	m_color(color),
	m_fillcolor(fillcolor)
{
}

void Artifact::SetColor(COLORREF color)
{
	m_color = color;
}

void Artifact::SetFillColor(COLORREF color)
{
	m_fillcolor = color;
}

COLORREF Artifact::GetColor() const
{
	return m_color;
}

COLORREF Artifact::GetFillColor() const
{
	return m_fillcolor;
}

int Artifact::GetPosition() const
{
	return m_position;
}

Line::Line(const std::wstring& name) :
	m_name(name)
{
}

std::wstring Line::GetName() const
{
	return m_name;
}

void Line::Add(Artifact artifact)
{
	m_artifacts.emplace_back(artifact);
}

std::vector<Artifact> Line::GetArtifacts() const
{
	return m_artifacts;
}

// start = 200, end = 700, anchor = Left, minorTicksPerMajorTick = 5, minorTickSize = 10, unit = "ms"
void CTimelineView::SetView(Location start, Location end, Anchor anchor, int minorTicksPerMajorTick, Location minorTickSize, const std::wstring unit)
{
	m_start = start;		// start of the scale (the scale maybe become larger, but at least this much will fit)
	m_end = end;			// end of the scale (the scale maybe become larger, but at least this much will fit)
	m_anchor = anchor;		// anchor location (left, right, center)
	m_minorTicksPerMajorTick = minorTicksPerMajorTick;
	m_minorTickSize = minorTickSize;
	m_unit = unit;
	m_cursorX = 0;
	m_viewWidth = 0;
	m_minorTickPixels = 0;
}

void CTimelineView::Recalculate(graphics::TimelineDC& dc)
{
	m_viewWidth = dc.GetClientArea().right - graphics::s_drawTimelineMax;
	int pixelsPerLocation = m_viewWidth / (m_end - m_start);
	assert((pixelsPerLocation > 1) && "This egde-case has not been implemented");
	m_minorTickPixels = pixelsPerLocation * m_minorTickSize;
}

LONG CTimelineView::GetTrackPos32(int nBar)
{
	SCROLLINFO si = { sizeof(si), SIF_TRACKPOS };
	GetScrollInfo(nBar, &si);
	return si.nTrackPos;
}

void CTimelineView::OnMouseMove(UINT nFlags, CPoint point)
{
	m_cursorX = point.x;
	Invalidate();
}

BOOL CTimelineView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	cdbg << "OnMouseWheel, zDelta: " << zDelta << "\n";			// todo: find out why these events are not captured like in CMainFrame::OnMouseWheel
	return TRUE;
}

void CTimelineView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
{
	if (nSBCode == SB_THUMBTRACK)
	{
		cdbg << "OnHScroll, nPos: " << nPos << "\n";	// received range is 1-100
		SetScrollPos(SB_HORZ, nPos);
		Invalidate();
	}
}

int CTimelineView::GetXforPosition(graphics::TimelineDC& dc, int pos) const
{
	assert((pos >= m_start && pos <= m_end) && "position not in current range");
	cdbg << " client width: " << m_viewWidth << "\n";
	cdbg << " (m_end - m_start): " << (m_end - m_start) << "\n";

	auto result = graphics::s_drawTimelineMax + (((pos - m_start) * m_viewWidth) / (m_end - m_start));
	cdbg << " pos: " << pos << ", result: " << result << "\n";
	return result;
}

BOOL CTimelineView::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	SetScrollRange(SB_HORZ, 1, 500);	// no effect??
	return 1;
}

void CTimelineView::PaintScale(graphics::TimelineDC& dc)
{
	assert((m_viewWidth != 0) && (m_minorTickPixels !=0) && "Recalculate must be called before PaintScale()");
	auto width = dc.GetClientArea().right - graphics::s_drawTimelineMax;
	int y = 25;
	int x = graphics::s_drawTimelineMax;

	int minorTicks = width / m_minorTickPixels;
	for (int i = 0; i < minorTicks; ++i)
	{
		dc.MoveTo(x, y);
		dc.LineTo(x, y - 3);
		x += m_minorTickPixels;
	}

	x = graphics::s_drawTimelineMax;
	int pos = m_start;
	int majorTicks = width / (m_minorTicksPerMajorTick * m_minorTickPixels);
	for (int i = 0; i < majorTicks; ++i)
	{
		std::wstring s = wstringbuilder() << pos << m_unit;
		dc.DrawTextOut(s, x - 15, y - 25);
		dc.MoveTo(x, y);
		dc.LineTo(x, y - 7);
		x += (m_minorTicksPerMajorTick * m_minorTickPixels);
		pos += (m_minorTicksPerMajorTick * m_minorTickSize);
	}
}

void CTimelineView::PaintCursor(graphics::TimelineDC& dc)
{
	auto rect = dc.GetClientArea();
	dc.Rectangle(m_cursorX, 0, m_cursorX+1, rect.bottom);
}

void CTimelineView::PaintTimelines(graphics::TimelineDC& dc)
{
	auto rect = dc.GetClientArea();

	int y = 50;
	y += GetTrackPos32(SB_HORZ);
	for (auto& line : m_lines)
	{
		auto grey = RGB(160, 160, 170);
		dc.DrawTimeline(line.GetName(), 0, y, rect.right - 200, grey);
		for (auto& artifact : line.GetArtifacts())
		{
			dc.DrawSolidFlag(L"tag", GetXforPosition(dc, artifact.GetPosition()), y, artifact.GetColor(), artifact.GetFillColor());
		}
		y += 25;
	}
	
	auto grey = RGB(160, 160, 170);
	dc.DrawTimeline(L"Move Sequence", 0, y, 500, grey);
	dc.DrawFlag(L"tag", 200, y);
	dc.DrawFlag(L"tag", 250, y);
	dc.DrawSolidFlag(L"tag", 260, y, RGB(255, 0, 0), RGB(0, 255, 0));
	dc.DrawFlag(L"tag", 270, y);

	y += 25;
	dc.DrawTimeline(L"Arbitrary data", 0, y, 500, grey);
	dc.DrawFlag(L"blueFlag", 470, y, RGB(0, 0, 255), true);
}

void CTimelineView::OnPaint(CDCHandle cdc)	// why is this cdc broken? contains a nullptr
{
	using namespace fusion;
	PAINTSTRUCT ps;
	BeginPaint(&ps);
	graphics::TimelineDC dc(GetDC());

	auto rect = dc.GetClientArea();
	dc.Rectangle(&rect);

	Recalculate(dc);
	PaintScale(dc);
	PaintTimelines(dc);
	PaintCursor(dc);
	EndPaint(&ps);
}

Line& CTimelineView::Add(const std::string& name)
{
	m_lines.emplace_back(WStr(name));
	return m_lines.back();
}

} // namespace graphics
} // namespace fusion
