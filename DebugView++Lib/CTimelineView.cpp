// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

// #include "stdafx.h"
#include "DebugView++Lib/CTimelineView.h"
#include <string>
#include <iomanip>
#include <cassert>
#include "CobaltFusion/dbgstream.h"
#include "CobaltFusion/Str.h"
#include "CobaltFusion/stringbuilder.h"
#include <algorithm>

namespace fusion {
namespace gdi {

Artifact::Artifact(Pixel position, Artifact::Type type) :
    m_position(position),
    m_type(type),
    m_color(RGB(0, 0, 0)),
    m_fillcolor(RGB(255, 255, 255))
{
}

Artifact::Artifact(Pixel position, Artifact::Type type, COLORREF color) :
    m_position(position),
    m_type(type),
    m_color(color),
    m_fillcolor(RGB(255, 255, 255))
{
}

Artifact::Artifact(Pixel position, Artifact::Type type, COLORREF color, COLORREF fillcolor) :
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
    SCROLLINFO si = {sizeof(si), SIF_TRACKPOS};
    GetScrollInfo(nBar, &si);
    return si.nTrackPos;
}

void CTimelineView::OnMouseMove(UINT nFlags, CPoint point)
{
    m_cursorPosition = std::max(int(point.x), gdi::s_leftTextAreaBorder);
    SetFocus();
    Invalidate();
}

BOOL CTimelineView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    m_mouseScrollCallback(m_cursorPosition - gdi::s_leftTextAreaBorder, m_selectedPosition - gdi::s_leftTextAreaBorder, zDelta);
    Invalidate();
    return TRUE;
}

void CTimelineView::OnLButtonDown(UINT flags, CPoint point)
{
    m_selectedPosition = std::max(int(point.x), gdi::s_leftTextAreaBorder);
    SetFocus();
    Invalidate();
}

void CTimelineView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
{
    if (nSBCode == SB_THUMBTRACK)
    {
        cdbg << "TRACK OnHScroll, nPos: " << nPos << "\n"; // received range is 1-100, unaffected by SetScrollRange ?
        SetScrollPos(SB_HORZ, nPos);
        Invalidate();
    }
}

BOOL CTimelineView::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
    //SetScrollRange(SB_HORZ, 1, 500);    // no effect??
    return 1;
}

void CTimelineView::DoPaint(CDCHandle cdc)
{
    using namespace fusion;
    gdi::TimelineDC dc(cdc);
    auto backgroundArea = dc.GetClientArea();
    dc.FillSolidRect(&backgroundArea, RGB(255, 255, 255));

    m_timelines = Recalculate(dc);

    PaintScale(dc);
    PaintTimelines(dc);
    PaintCursors(dc);
}

void CTimelineView::SetFormatter(FormatFunction f)
{
    m_formatFunction = f;
}

void CTimelineView::SetDataProvider(DataProvider f)
{
    m_dataProvider = f;
}

void CTimelineView::SetMouseScrollCallback(MouseScrollCallback f)
{
    m_mouseScrollCallback = f;
}

TimeLines CTimelineView::Recalculate(gdi::TimelineDC& dc)
{
    return m_dataProvider();
}

static const Pixel scaleBottom = 25;
static const Pixel topTimelinePos = 50;
static const Pixel minorTickHeight = 3;
static const Pixel majorTickHeight = 7;

void CTimelineView::PaintScale(gdi::TimelineDC& dc)
{
    Pixel width = dc.GetClientArea().right - gdi::s_leftTextAreaBorder;
    int y = scaleBottom;
    auto x = gdi::s_leftTextAreaBorder + m_tickOffset;
    int lastX = 0;

    //auto pos = m_tickOffset;
    int majorTicks = width / m_minorTicksPerMajorTick;
    for (int i = 0; i < majorTicks; ++i)
    {
        std::wstring s = m_formatFunction(x - gdi::s_leftTextAreaBorder);
        dc.DrawTextOut(s, x - 15, 0);
        dc.MoveTo(x, y);
        dc.LineTo(x, y - majorTickHeight);
        lastX = x;
        x += (m_minorTicksPerMajorTick * m_minorTickSize);
    }

    x = gdi::s_leftTextAreaBorder;
    //int minorTicks = width / m_minorTickSize;
    for (; x < lastX;)
    {
        dc.MoveTo(x, y);
        dc.LineTo(x, y - minorTickHeight);
        x += m_minorTickSize;
    }
}

void CTimelineView::PaintCursors(gdi::TimelineDC& dc)
{
    auto rect = dc.GetClientArea();

    CPen redpen(CreatePen(PS_SOLID, 1, RGB(255, 0, 0)));
    CPen greenpen(CreatePen(PS_SOLID, 1, RGB(0, 255, 0)));

    dc.SelectPen(redpen);
    dc.MoveTo(m_cursorPosition, scaleBottom);
    dc.LineTo(m_cursorPosition, rect.bottom);

    dc.SelectPen(greenpen);
    dc.MoveTo(m_selectedPosition, scaleBottom);
    dc.LineTo(m_selectedPosition, rect.bottom);
}

void CTimelineView::PaintTimelines(gdi::TimelineDC& dc)
{
    auto rect = dc.GetClientArea();

    int y = topTimelinePos;
    y += GetTrackPos32(SB_HORZ); //test
    for (const auto& line : m_timelines)
    {
        auto grey = RGB(160, 160, 170);
        dc.DrawTimeline(line->GetName(), 0, y, rect.right, grey);
        for (auto& artifact : line->GetArtifacts())
        {
            //switch (artifact.
            dc.DrawSolidFlag(L"tag", artifact.GetPosition() + gdi::s_leftTextAreaBorder, y, artifact.GetColor(), artifact.GetFillColor());
        }
        y += 25;
    }
}


} // namespace gdi
} // namespace fusion
