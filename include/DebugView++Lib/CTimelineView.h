// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "DebugView++Lib/TimelineDC.h"
#include "CobaltFusion/Math.h"

#include <string>
#include <vector>
#include <memory>
#include <functional>

#include "windows.h"

#include "atlapp.h"
#include "atlgdi.h"
#include "atlframe.h"
#include "atlcrack.h"
#include "atlctrls.h"
#include "atlmisc.h"
#include "atlscrl.h"

namespace fusion {
namespace gdi {

// see https://www.codeproject.com/Articles/12999/WTL-for-MFC-Programmers-Part-IX-GDI-Classes-Common
// https://www.codeproject.com/KB/wtl/#Beginners

class Artifact
{
public:
    Artifact() = default;
    Artifact(const Artifact&) = default;
    Artifact& operator=(const Artifact&) = default;

    Artifact(Artifact&&) = default;
    Artifact& operator=(Artifact&&) = default;

    enum class Type
    {
        Flag,
        StartStopEvent,
        SelectCursor
    };

    Artifact(Pixel position, Artifact::Type type);
    Artifact(Pixel position, Artifact::Type type, COLORREF color);
    Artifact(Pixel position, Artifact::Type type, COLORREF color, COLORREF fillcolor);
    Pixel GetPosition() const;

    void SetColor(COLORREF color);
    void SetFillColor(COLORREF color);
    COLORREF GetColor() const;
    COLORREF GetFillColor() const;

private:
    Pixel m_position;
    Type m_type;
    COLORREF m_color;
    COLORREF m_fillcolor;
};

class Line
{
public:
    explicit Line(const std::wstring& name);

    Line() = delete;
    Line(const Line&) = delete;
    Line& operator=(const Line&) = delete;

    Line(Line&&) = default;
    Line& operator=(Line&&) = default;

    void Add(Artifact artifact);
    std::wstring GetName() const;
    std::vector<Artifact> GetArtifacts() const;

private:
    std::wstring m_name;
    std::vector<Artifact> m_artifacts;
};

using TimeLines = std::vector<std::shared_ptr<Line>>;

// zooming and panning is not part of the CTimelineView responsibility, it is a 'dumb' drawing class
class CTimelineView : public CDoubleBufferWindowImpl<CTimelineView, CWindow>
{
public:
    DECLARE_WND_CLASS(_T("CTimelineView Class"))

    BEGIN_MSG_MAP(CTimelineView)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_MOUSEWHEEL(OnMouseWheel)
        MSG_WM_MOUSEMOVE(OnMouseMove)
        MSG_WM_HSCROLL(OnHScroll)
        MSG_WM_LBUTTONDOWN(OnLButtonDown)
        CHAIN_MSG_MAP(CDoubleBufferImpl<CTimelineView>)
    END_MSG_MAP()

    BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
    void DoPaint(CDCHandle dc);
    BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar);
    void OnMouseMove(UINT nFlags, CPoint point);
    void OnLButtonDown(UINT flags, CPoint point);

    using FormatFunction = std::function<std::wstring(Pixel position)>;
    void SetFormatter(FormatFunction f);

    using MouseScrollCallback = std::function<void(Pixel cursorPosition, Pixel selectedPosition, int direction)>;
    void SetMouseScrollCallback(MouseScrollCallback f);

    using DataProvider = std::function<TimeLines()>;
    void SetDataProvider(DataProvider f);

private:
    TimeLines Recalculate(gdi::TimelineDC& dc);
    void PaintScale(gdi::TimelineDC& dc);
    void PaintTimelines(gdi::TimelineDC& dc);
    void PaintCursors(gdi::TimelineDC& dc);
    LONG GetTrackPos32(int nBar);

    Pixel m_minorTickSize = 10;
    Pixel m_minorTicksPerMajorTick = 10;
    Pixel m_tickOffset = 0;
    Pixel m_cursorPosition = 0;
    Pixel m_selectedPosition = 0;

    FormatFunction m_formatFunction;
    DataProvider m_dataProvider;
    TimeLines m_timelines;
    MouseScrollCallback m_mouseScrollCallback;
};


} // namespace gdi
} // namespace fusion
