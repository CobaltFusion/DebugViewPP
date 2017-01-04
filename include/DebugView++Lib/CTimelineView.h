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
#include "boost/noncopyable.hpp"

namespace fusion {
namespace gdi {

// see https://www.codeproject.com/Articles/12999/WTL-for-MFC-Programmers-Part-IX-GDI-Classes-Common
// https://www.codeproject.com/KB/wtl/#Beginners

class Artifact
{
public:
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

class Line // : boost::noncopyable
{
public:
	explicit Line(const std::wstring& name);
	void Add(Artifact artifact);
	std::wstring GetName() const;
	std::vector<Artifact> GetArtifacts() const;

private:
	std::wstring m_name;
	std::vector<Artifact> m_artifacts;
};

using Pixel = int;
using Location = int;

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
		CHAIN_MSG_MAP(CDoubleBufferImpl<CTimelineView>)		//DrMemory: GDI USAGE ERROR: DC 0x3e011cca that contains selected object being deleted
	END_MSG_MAP()

	void SetView(Location start, Location end, const std::wstring unit);

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void DoPaint(CDCHandle dc);
	BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar);

	void OnMouseMove(UINT nFlags, CPoint point);

	Line& Add(const std::string& name);
	void Zoom(double factor);
private:
	void PaintScale(graphics::TimelineDC& dc);
	void PaintTimelines(graphics::TimelineDC& dc);
	void PaintCursor(graphics::TimelineDC& dc);
	LONG GetTrackPos32(int nBar);
	Pixel GetX(Location pos) const;
	bool InRange(Location pos) const;
	void Recalculate(graphics::TimelineDC& dc);

	// input
	int m_start;
	int m_end;
	int m_minorTickSize;
	int m_minorTicksPerMajorTick;

	// calculated 
	int m_minorTickPixels = 0;
	int m_viewWidth = 0;
	int m_pixelsPerLocation = 0;

	LONG m_cursorX = 0;
	std::wstring m_unit;
	SCROLLINFO m_scrollInfo;
	std::vector<Line> m_lines;
};


} // namespace gdi
} // namespace fusion
