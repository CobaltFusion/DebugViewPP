// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "windows.h"
#include <string>
#include <vector>
#include "atlapp.h"
#include "atlgdi.h"
#include "atlframe.h"
#include "atlcrack.h"
#include "atlctrls.h"
#include "atlmisc.h"
#include "atlscrl.h"
#include "DebugView++Lib/TimelineDC.h"
#include "CobaltFusion/Math.h"
#include "boost/core/noncopyable.hpp"
#include <memory>
#include <functional>

namespace fusion {
namespace gdi {

// see https://www.codeproject.com/Articles/12999/WTL-for-MFC-Programmers-Part-IX-GDI-Classes-Common
// https://www.codeproject.com/KB/wtl/#Beginners

class Artifact
{
public:
	Artifact() = default;
	Artifact(const Artifact&) = default;
	Artifact& operator = (const Artifact&) = default;

	Artifact(Artifact&&) = default;
	Artifact& operator = (Artifact&&) = default;

	enum class Type { Flag, StartStopEvent };

	Artifact(int position, Artifact::Type type);
	Artifact(int position, Artifact::Type type, COLORREF color);
	Artifact(int position, Artifact::Type type, COLORREF color, COLORREF fillcolor);
	int GetPosition() const;

	void SetColor(COLORREF color);
	void SetFillColor(COLORREF color);
	COLORREF GetColor() const;
	COLORREF GetFillColor() const;
private:
	int m_position;
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
	Line& operator = (const Line&) = delete;

	Line(Line&&) = default;
	Line& operator = (Line&&) = default;

	void Add(Artifact artifact);
	std::wstring GetName() const;
	std::vector<Artifact> GetArtifacts() const;

private:
	std::wstring m_name;
	std::vector<Artifact> m_artifacts;
};

using TimeLines = std::vector<std::shared_ptr<Line>>;

// zooming and panning is not part of the CTimelineView responsibility.
// it is a 'dumb' drawing class that deals with positioning and formatting
// it has no concept of time, just position which is scaled to window-pixels.
// also no centering or end-of-range behaviour is implemented, this is client responsibility.
class CTimelineView : 
	public CDoubleBufferWindowImpl<CTimelineView, CWindow>,
	public COwnerDraw<CTimelineView>
{
public:
	DECLARE_WND_CLASS(_T("CTimelineView Class"))

	BEGIN_MSG_MAP(CTimelineView)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_MOUSEWHEEL(OnMouseWheel)
		MSG_WM_MOUSEMOVE(OnMouseMove)
		MSG_WM_HSCROLL(OnHScroll)
		CHAIN_MSG_MAP_ALT(COwnerDraw<CTimelineView>, 1)
		CHAIN_MSG_MAP(CDoubleBufferImpl<CTimelineView>)
	END_MSG_MAP()

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void DoPaint(CDCHandle dc);
	BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar);
	void OnMouseMove(UINT nFlags, CPoint point);

	void SetView(Location start, Location end);
	void Zoom(double factor);

	using formatFunction = std::function<std::string(Location)>;
	void SetFormatter(formatFunction f);

	using dataProvider = std::function<TimeLines(Location start, Location end)>;
	void SetDataProvider(dataProvider f);

private:
	void Recalculate(gdi::TimelineDC& dc);
	void PaintScale(gdi::TimelineDC& dc);
	void PaintTimelines(gdi::TimelineDC& dc);
	void PaintCursor(gdi::TimelineDC& dc);
	LONG GetTrackPos32(int nBar);
	Pixel GetX(Location pos) const;
	bool InRange(Location pos) const;

	// input
	double m_start = 0;
	double m_end = 0;
	double m_minorTickSize = 0;
	double m_minorTicksPerMajorTick = 0;
	double m_tickOffset = 0;

	// calculated 
	double m_minorTickPixels = 0;
	int m_viewWidth = 0;
	double m_pixelsPerLocation = 0;

	LONG m_cursorX = 0;
	SCROLLINFO m_scrollInfo;
	formatFunction m_formatFunction;
	dataProvider m_dataProvider;
};


} // namespace gdi
} // namespace fusion
