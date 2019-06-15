// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <memory>

#include "atlgdix.h"
#include "atlcoll.h"
namespace WTL {
using ATL::CString;
}

#include <atlcom.h>

#pragma warning(push, 3)
#pragma warning(disable : 4838)

#include "CustomTabCtrl.h"
#include "DotNetTabCtrl.h"
#include "TabbedFrame.h"
#pragma warning(pop)

#include "DebugView++Lib/CTimelineView.h"

namespace fusion {
namespace debugviewpp {

class CLogView;

class CLogViewTabItem : public CTabViewTabItem
{
public:
	void SetView(std::shared_ptr<CLogView> pView);
	CLogView& GetView();

	void Create(HWND parent) { m_parent = parent; }
	HWND GetLogViewParent() { return m_parent; }

private:
	std::shared_ptr<CLogView> m_pView;
	HWND m_parent = HWND();
};

class CMyPaneContainer : public CPaneContainerImpl<CMyPaneContainer>
{
public:
	DECLARE_WND_CLASS_EX(_T("MY_PaneContainer"), 0, -1)

	BEGIN_MSG_MAP(CMyPaneContainerImpl)
		REFLECT_NOTIFICATIONS()
		CHAIN_MSG_MAP(CPaneContainerImpl<CMyPaneContainer>)
	END_MSG_MAP()
};

using TimePoint = std::chrono::steady_clock::time_point;
using Duration = std::chrono::steady_clock::duration;

class ViewPort
{
public:
	ViewPort() {}
	ViewPort(TimePoint begin, TimePoint end, Duration timeUnitPerPixel);
	bool Contains(TimePoint p) const;
	gdi::Pixel ToPx(TimePoint p) const;
	Duration ToDuration(gdi::Pixel p) const;
	TimePoint ToTimePoint(gdi::Pixel p) const;
	std::wstring FormatAsTime(gdi::Pixel p);

	void ZoomInTo(gdi::Pixel position);
	void ZoomOut(gdi::Pixel position);
private:

	// time range
	TimePoint m_begin;
	TimePoint m_end;

	// conversion factor
	Duration m_timeunitPerPixelBase;
	Duration m_timeunitPerPixel;
	int m_zoomFactor = 1000;
};

class CLogViewTabItem2 : public CTabViewTabItem
{
public:
	~CLogViewTabItem2();
	void SetView(std::shared_ptr<CLogView> pView);
	CLogView& GetView();

	void Create(HWND parent);
	HWND GetLogViewParent() { return m_top; }

private:
	std::shared_ptr<CLogView> m_pView;
	CHorSplitterWindow m_split;
	CMyPaneContainer m_top;
	CMyPaneContainer m_bottom;
	gdi::CTimelineView m_timelineView;
	ViewPort m_viewPort;
};


} // namespace debugviewpp
} // namespace fusion
