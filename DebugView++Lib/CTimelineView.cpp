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
	assert(((end - start) > 0) && "view range must be > 0");
	m_start = start;		// start of the scale (the scale maybe become larger, but at least this much will fit)
	m_end = end;			// end of the scale (the scale maybe become larger, but at least this much will fit)
	m_anchor = anchor;		// anchor location (left, right, center)
	m_minorTicksPerMajorTick = minorTicksPerMajorTick;
	m_minorTickSize = minorTickSize;
	m_unit = unit;
	Invalidate();
}

void CTimelineView::Recalculate(graphics::TimelineDC& dc)
{
	m_viewWidth = dc.GetClientArea().right - graphics::s_drawTimelineMax;
	m_pixelsPerLocation = m_viewWidth / (m_end - m_start);
	assert((m_pixelsPerLocation > 1) && "This egde-case has not been implemented");
	m_minorTickPixels = m_pixelsPerLocation * m_minorTickSize;
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
	cdbg << "CTimelineView::OnMouseMove, m_cursorX: " << m_cursorX << "\n";
	Invalidate();
}

BOOL CTimelineView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	cdbg << "CTimelineView::OnMouseWheel, zDelta: " << zDelta << "\n";			// todo: find out why these events are not captured like in CMainFrame::OnMouseWheel
	return TRUE;
}

void CTimelineView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
{
	if (nSBCode == SB_THUMBTRACK)
	{
		cdbg << "OnHScroll, nPos: " << nPos << "\n";	// received range is 1-100, unaffected by SetScrollRange ?
		SetScrollPos(SB_HORZ, nPos);
		Invalidate();
	}
}

Pixel CTimelineView::GetX(Location pos) const
{
	assert(InRange(pos) && "position not in current range");
	return graphics::s_drawTimelineMax + (m_pixelsPerLocation * (pos - m_start));
}

bool CTimelineView::InRange(Location pos) const
{
	return (pos >= m_start && pos <= m_end);
}

BOOL CTimelineView::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	SetScrollRange(SB_HORZ, 1, 500);	// no effect??
	return 1;
}

void CTimelineView::PaintScale(graphics::TimelineDC& dc)
{
	assert((m_viewWidth != 0) && (m_minorTickPixels != 0) && (m_pixelsPerLocation != 0) && "Recalculate must be called before PaintScale()");
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
			if (InRange(artifact.GetPosition()))
			{
				dc.DrawSolidFlag(L"tag", GetX(artifact.GetPosition()), y, artifact.GetColor(), artifact.GetFillColor());
			}
			
		}
		y += 25;
	}
}

void CTimelineView::DoPaint(CDCHandle cdc)
{
	using namespace fusion;
	graphics::TimelineDC dc(cdc);
	auto backgroundArea = dc.GetClientArea();
	dc.FillSolidRect(&backgroundArea, RGB(255, 255, 255));

	Recalculate(dc);
	PaintScale(dc);
	PaintTimelines(dc);
	PaintCursor(dc);
}

Line& CTimelineView::Add(const std::string& name)
{
	m_lines.emplace_back(WStr(name));
	return m_lines[m_lines.size() - 1];
}

} // namespace graphics
} // namespace fusion
