// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DebugView++Lib/CTimelineView.h"
#include <string>
#include <iomanip>
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

Pixel Artifact::GetPosition() const
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

LONG CTimelineView::GetTrackPos32(int nBar)
{
	SCROLLINFO si = { sizeof(si), SIF_TRACKPOS };
	GetScrollInfo(nBar, &si);
	return si.nTrackPos;
}

void CTimelineView::OnMouseMove(UINT nFlags, CPoint point)
{
	m_cursorPosition = point.x;
	SetFocus();
	Invalidate();
}

int GetOffsetTillNextMultiple(int value, int multiplier)
{
	auto modulus = value % multiplier;
	if (modulus)
		return (multiplier - modulus);
	return 0;
	
}

BOOL CTimelineView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// todo: find out why these events are only captured is the focus returns after opening and closing the filter-dialog
	// but not when clicking on the timeline
	cdbg << "CAP CTimelineView::OnMouseWheel, zDelta: " << zDelta << "\n";			
	return TRUE;
}

void CTimelineView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
{
	if (nSBCode == SB_THUMBTRACK)
	{
		cdbg << "TRACK OnHScroll, nPos: " << nPos << "\n";	// received range is 1-100, unaffected by SetScrollRange ?
		SetScrollPos(SB_HORZ, nPos);
		Invalidate();
	}
}

BOOL CTimelineView::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	//SetScrollRange(SB_HORZ, 1, 500);	// no effect??
	return 1;
}

void CTimelineView::DoPaint(CDCHandle cdc)
{
	using namespace fusion;
	gdi::TimelineDC dc(cdc);
	auto backgroundArea = dc.GetClientArea();
	dc.FillSolidRect(&backgroundArea, RGB(255, 255, 255));

	if (backgroundArea.right!= m_viewWidth)
	{
		// width changed
		m_viewWidth = backgroundArea.right;
		m_timelines = Recalculate(dc);
	}

	PaintScale(dc);
	PaintTimelines(dc);
	PaintCursor(dc);
}

void CTimelineView::SetFormatter(FormatFunction f)
{
	m_formatFunction = f;
}

void CTimelineView::SetDataProvider(DataProvider f)
{
	m_dataProvider = f;
}

TimeLines CTimelineView::Recalculate(gdi::TimelineDC& dc)
{
	auto timelineWidth = dc.GetClientArea().right - gdi::s_leftTextAreaBorder;
	return m_dataProvider(timelineWidth, m_cursorPosition);
}

void CTimelineView::PaintScale(gdi::TimelineDC& dc)
{
	// zooming should be done by: 
	// - changing the m_pixelsPerLocation value
	// - keeping the cursur position fixed (pixel precise)
	// - offsetting the scale to match the timeline, not the other way around, this way the numbers on the scale can always remain powers of 10 
	//   while the cursor-position is some arbitrary value.

	// zooming does not work because PaintScale() always starts at m_start and we now have a fixed  m_minorTickSize = 15; m_minorTicksPerMajorTick = 5;
	// PaintScale() must draw the scale relative to a fixed zoompoint and in such a way that the pixel-location of the zoompoint stays exactly fixed and scale
	// is fitted around it accoording to the available view-size.

	//assert((m_viewWidth != 0) && (m_minorTickPixels != 0) && (m_pixelsPerLocation != 0) && "Recalculate must be called before PaintScale()");
	Pixel width = dc.GetClientArea().right - gdi::s_leftTextAreaBorder;
	int y = 25;
	auto x = gdi::s_leftTextAreaBorder + m_tickOffset;
	int lastX = 0;

	auto pos = m_tickOffset;
	int majorTicks = width / m_minorTicksPerMajorTick;
	for (int i = 0; i < majorTicks; ++i)
	{
		std::wstring s = m_formatFunction(x);
		dc.DrawTextOut(s, x - 15, y - 25);
		dc.MoveTo(x, y);
		dc.LineTo(x, y - 7);
		lastX = x;
		x += (m_minorTicksPerMajorTick * m_minorTickSize);
	}

	x = gdi::s_leftTextAreaBorder;
	int minorTicks = width / m_minorTickSize;
	for (;x < lastX;)
	{
		dc.MoveTo(x, y);
		dc.LineTo(x, y - 3);
		x += m_minorTickSize;
	}
}

void CTimelineView::PaintCursor(gdi::TimelineDC& dc)
{
	auto rect = dc.GetClientArea();
	dc.Rectangle(m_cursorPosition, 0, m_cursorPosition +1, rect.bottom);
}

void CTimelineView::PaintTimelines(gdi::TimelineDC& dc)
{
	auto rect = dc.GetClientArea();

	int y = 50;
	y += GetTrackPos32(SB_HORZ);
	for (auto line : m_timelines)
	{
		auto grey = RGB(160, 160, 170);
		dc.DrawTimeline(line->GetName(), 0, y, rect.right - 200, grey);
		for (auto& artifact : line->GetArtifacts())
		{
			dc.DrawSolidFlag(L"tag", artifact.GetPosition(), y, artifact.GetColor(), artifact.GetFillColor());
		}
		y += 25;
	}
}


} // namespace gdi
} // namespace fusion
