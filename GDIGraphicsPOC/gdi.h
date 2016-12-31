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
#include "Win32/gdi.h"

namespace fusion {
namespace gdi {

// see https://www.codeproject.com/Articles/12999/WTL-for-MFC-Programmers-Part-IX-GDI-Classes-Common
// https://www.codeproject.com/KB/wtl/#Beginners

class DeviceContextEx : public CWindowDC
{
	using CWindowDC::CWindowDC;
public:
	// wrappers
	BOOL DrawPolygon(const std::vector<POINT>& points);
	BOOL DrawTextOut(const std::wstring& str, int x, int y);

	// extentions
	void DrawTimeline(const std::wstring& name, int x, int y, int width, COLORREF color);
	void DrawFlag(const std::wstring& /* tooltip */, int x, int y);
	void DrawSolidFlag(const std::wstring& /* tooltip */, int x, int y);
	void DrawSolidFlag(const std::wstring& /* tooltip */, int x, int y, COLORREF border, COLORREF fill);
	void DrawFlag(const std::wstring& /* tooltip */, int x, int y, COLORREF color, bool solid);
};

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

class Line
{
public:
	Line(const std::wstring& name);
	void Add(Artifact artifact);
	std::wstring GetName() const;
	std::vector<Artifact> GetArtifacts() const;

private:
	std::wstring m_name;
	std::vector<Artifact> m_artifacts;
};

// zooming and panning is not part of the CTimelineView responsibility.
// it is a 'dumb' drawing class that deals with positioning and formatting
// it has no concept of time, just position which is scaled to window-pixels.
class CTimelineView : public CWindowImpl<CTimelineView, CWindow>
{
public:
	DECLARE_WND_CLASS(_T("CTimelineView Class"))

	BEGIN_MSG_MAP(CTimelineView)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_PAINT(OnPaint)
		MSG_WM_MOUSEWHEEL(OnMouseWheel)
		MSG_WM_HSCROLL(OnHScroll)
	END_MSG_MAP()

	void Initialize(int start, int end, int minorTicksPerMajorTick, int minorTickSize, int minorTickPixels, const std::wstring unit);

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnPaint(CDCHandle dc);
	BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar);

	Line& Add(const std::string& name);

private:
	void PaintScale(graphics::DeviceContextEx& dc);
	void PaintTimelines(graphics::DeviceContextEx& dc);
	LONG GetTrackPos32(int nBar);

	int m_start;
	int m_end;
	int m_minorTickSize;
	int m_minorTicksPerMajorTick;
	int m_minorTickPixels;
	std::wstring m_unit;
	SCROLLINFO m_scrollInfo;
	std::vector<Line> m_lines;
};

} // namespace gdi
} // namespace fusion
