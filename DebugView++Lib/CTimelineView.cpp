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

int GetOffsetTillNextMultiple(int value, int multiplier)
{
	auto modulus = value % multiplier;
	if (modulus)
		return (multiplier - modulus);
	return 0;
	
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

Pixel ToPixel(double value)
{
	return static_cast<int>(std::floor(value));
}

Pixel CTimelineView::GetX(Location pos) const
{
	assert(InRange(pos) && "position not in current range");
	return ToPixel(gdi::s_drawTimelineMax + (m_pixelsPerLocation * (pos - m_start)));
}

bool CTimelineView::InRange(Location pos) const
{
	return (pos >= m_start && pos <= m_end);
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

	Recalculate(dc);
	PaintScale(dc);
	PaintTimelines(dc);
	PaintCursor(dc);
}

void CTimelineView::SetFormatter(formatFunction f)
{
	m_formatFunction = f;
}

void CTimelineView::SetDataProvider(dataProvider f)
{
	m_dataProvider = f;
}

void CTimelineView::SetView(Location start, Location end)
{
	assert(((end - start) > 0) && "view range must be > 0");
	m_start = start;		// start of the scale (the scale maybe become larger, but at least this much will fit)
	m_end = end;			// end of the scale (the scale maybe become larger, but at least this much will fit)

	m_pixelsPerLocation = 0;		// will cause Recalculate() to re-initialize it
	m_minorTickPixels = 10;			// fixed
	m_minorTicksPerMajorTick = 10;	// fixed
	m_tickOffset = 0;

	m_minorTickSize = 10;				//todo: must not be fixed
	Invalidate();
}

double relativeFloor(double value)
{
	return value;
	//auto e = std::floor(std::log10(std::abs(value)));
	//auto base = value / std::pow(10, e);
	//return std::pow(10, e) * std::floor(base);
}

void CTimelineView::Zoom(double factor)
{
	auto zoomCenter = m_start + ((m_cursorX - gdi::s_drawTimelineMax) / m_pixelsPerLocation);
	cdbg << "zoomCenter: " << zoomCenter << "\n";
	cdbg << "m_start: " << m_start << "\n";
	cdbg << "m_end: " << m_end << "\n";

	auto relStart = m_start - zoomCenter;
	auto relEnd = m_end - zoomCenter;

	cdbg << "relStart: " << relStart << "\n";
	cdbg << "relEnd: " << relEnd << "\n";

	auto relStartPx = relStart * m_pixelsPerLocation;
	auto relEndPx = relEnd * m_pixelsPerLocation;

	m_pixelsPerLocation = m_pixelsPerLocation * factor;

	relStart = relStartPx / m_pixelsPerLocation;
	relEnd = relEnd / m_pixelsPerLocation;

	m_start = relativeFloor(relStart + zoomCenter);
	m_end = relativeFloor(relEnd + zoomCenter);

//	m_tickOffset = GetOffsetTillNextMultiple(m_start, m_minorTicksPerMajorTick * m_minorTickSize);

	cdbg << "    m_pixelsPerLocation: " << m_pixelsPerLocation << "\n";
	cdbg << "    m_start: " << m_start << "\n";
	cdbg << "    m_end: " << m_end << "\n";

	Invalidate();
}

void CTimelineView::Recalculate(gdi::TimelineDC& dc)
{
	m_viewWidth = dc.GetClientArea().right - gdi::s_drawTimelineMax;
	if (m_pixelsPerLocation == 0)
		m_pixelsPerLocation = m_viewWidth / (m_end - m_start);
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

	assert((m_viewWidth != 0) && (m_minorTickPixels != 0) && (m_pixelsPerLocation != 0) && "Recalculate must be called before PaintScale()");
	Pixel width = dc.GetClientArea().right - gdi::s_drawTimelineMax;
	int y = 25;
	auto x = gdi::s_drawTimelineMax + FloorTo<int>(m_tickOffset * m_pixelsPerLocation);
	int lastX = 0;

	auto pos = m_start + m_tickOffset;
	int majorTicks = static_cast<int>((width / (m_minorTicksPerMajorTick * m_minorTickPixels)) + 1); // also add one at the end
	for (int i = 0; i < majorTicks; ++i)
	{
		std::wstring s = wstringbuilder() << m_formatFunction(pos);
		dc.DrawTextOut(s, x - 15, y - 25);
		dc.MoveTo(x, y);
		dc.LineTo(x, y - 7);
		lastX = x;
		x += static_cast<int>(m_minorTicksPerMajorTick * m_minorTickPixels);
		pos += (m_minorTicksPerMajorTick * m_minorTickSize);
	}

	x = gdi::s_drawTimelineMax;
	int minorTicks = static_cast<int>(width / m_minorTickPixels);
	for (;x < lastX;)
	{
		dc.MoveTo(x, y);
		dc.LineTo(x, y - 3);
		x += static_cast<int>(m_minorTickPixels);
	}
	m_end = ((lastX - gdi::s_drawTimelineMax) / m_pixelsPerLocation) + m_start;
}

void CTimelineView::PaintCursor(gdi::TimelineDC& dc)
{
	auto rect = dc.GetClientArea();
	dc.Rectangle(m_cursorX, 0, m_cursorX+1, rect.bottom);
}

void CTimelineView::PaintTimelines(gdi::TimelineDC& dc)
{
	auto rect = dc.GetClientArea();

	int y = 50;
	y += GetTrackPos32(SB_HORZ);
	for (auto line : m_dataProvider(m_start, m_end))
	{
		auto grey = RGB(160, 160, 170);
		dc.DrawTimeline(line->GetName(), 0, y, rect.right - 200, grey);
		for (auto& artifact : line->GetArtifacts())
		{
			if (InRange(artifact.GetPosition()))
			{
				dc.DrawSolidFlag(L"tag", GetX(artifact.GetPosition()), y, artifact.GetColor(), artifact.GetFillColor());
			}
		}
		y += 25;
	}
}


} // namespace gdi
} // namespace fusion
