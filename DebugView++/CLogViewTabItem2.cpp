// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "CLogViewTabItem2.h"
#include "CobaltFusion/stringbuilder.h"
#include "CobaltFusion/dbgstream.h"
#include "LogView.h"
#include <chrono>
#include <algorithm>
#include <iostream>

namespace fusion {
namespace debugviewpp {

using namespace std::chrono_literals;

void CLogViewTabItem::SetView(std::shared_ptr<CLogView> pView)
{
	m_pView = pView;
	SetTabView(*pView);
}

CLogView& CLogViewTabItem::GetView()
{
	return *m_pView;
}

void Add(const ViewPort& viewport, gdi::Line& line, gdi::Artifact a)
{
	if (viewport.Contains(TimePoint(a.GetPosition() * 1ms)))
	{
		line.Add(a);
	}
}

std::wstring FormatDuration(Duration d)
{
	if (d >= 1s) return wstringbuilder() << d.count() / 1e9 << L" s";
	if (d >= 1ms) return wstringbuilder() << d.count() / 1e6 << L" ms";
	if (d >= 1us) return wstringbuilder() << d.count() / 1e3 << L" us";
	return wstringbuilder() << d.count() << L" ns";
}
 
ViewPort::ViewPort(TimePoint begin, TimePoint end, Duration timeUnitPerPixel)
	: m_begin(begin)
	, m_end(end)
	, m_timeunitPerPixelBase(timeUnitPerPixel)
	, m_timeunitPerPixel(timeUnitPerPixel)
{
}

gdi::Pixel ViewPort::ToPx(TimePoint p) const
{
	assert((p >= m_begin) && "ToPx should only be called on TimePoints that where checked by Contains");
	auto d = p - m_begin;
	auto px = gdi::Pixel(d / m_timeunitPerPixel);
	return px;
}

Duration AsDuration(TimePoint p)
{
	return p.time_since_epoch();
}

// returns duration relative to the begin of the viewport.
Duration ViewPort::ToDuration(gdi::Pixel p) const
{
	return p * m_timeunitPerPixel;
}

// returns duration relative to the begin of the viewport.
TimePoint ViewPort::ToTimePoint(gdi::Pixel p) const
{
	return m_begin + (p * m_timeunitPerPixel);
}


bool ViewPort::Contains(TimePoint p) const
{
	return (p >= m_begin) && (p <= m_end);
}

std::wstring ViewPort::FormatAsTime(gdi::Pixel p)
{
	return FormatDuration(AsDuration(m_begin + ToDuration(p)));
}

// zooming should be done by: 
// - changing the m_timeunitPerPixel value
// - keeping the cursor position fixed (pixel precise)
// - offsetting the scale to match the timeline, not the other way around, this way the numbers on the scale can always remain powers of 10 

// Increase / Decrease sequence of 1, 2, 5, 10, 20, 50, 100, 200, etc...
Duration Increase(Duration d)
{
	Duration temp = d;
	while (temp > 10ns) temp /= 10;
	if (temp == 2ns) return (d * 2) + (d / 2);
	return d * 2;
}

Duration Decrease(Duration d)
{
	Duration temp = d;
	while (temp > 10ns) temp /= 10;
	if (temp == 5ns) return (d - (d / 5)) / 2;
	return std::max(1ns, d / 2);
}

TimePoint CorrectBegin(TimePoint begin, TimePoint t1, TimePoint t2)
{
	//return std::max(TimePoint(), begin + (t1 - t2));		// not correct, weird behaviour around 0
	return begin + (t1 - t2);
}

void ViewPort::ZoomInTo(gdi::Pixel position)
{
	auto t1 = ToTimePoint(position);
	//if (t1 < TimePoint(0ns)) return;
	m_timeunitPerPixel = Decrease(m_timeunitPerPixel);
	auto t2 = ToTimePoint(position);
	m_begin = CorrectBegin(m_begin, t1, t2);
}

void ViewPort::ZoomOut(gdi::Pixel position)
{
	auto t1 = ToTimePoint(position);
//	if (t1 < TimePoint(0ns)) return;
	m_timeunitPerPixel = Increase(m_timeunitPerPixel);
	auto t2 = ToTimePoint(position);
	m_begin = CorrectBegin(m_begin, t1, t2);
}

void DisablePaneHeader(CMyPaneContainer& panecontainer)
{
	panecontainer.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON, 0);
	panecontainer.m_cxyHeader = 0;
}

void CLogViewTabItem2::Create(HWND parent)
{
	m_split.Create(parent, CWindow::rcDefault);
	SetTabView(m_split);

	m_top.Create(m_split, L"");
	m_bottom.Create(m_split, L"");

	DisablePaneHeader(m_top);
	DisablePaneHeader(m_bottom);
	m_split.SetSplitterPanes(m_top, m_bottom, true);

	m_viewPort = ViewPort(TimePoint(0ms), TimePoint(5000ms), 500us);	// first two refer to the view on the input data, the third is a conversion factor

	m_timelineView.SetFormatter([&](gdi::Pixel position) {
		return m_viewPort.FormatAsTime(position);
	});

	m_timelineView.SetDataProvider([&]() {

		auto info = std::make_shared<gdi::Line>(L"Some info2");

		if (m_viewPort.Contains(TimePoint(350ms)))
		{
			Add(m_viewPort, *info, gdi::Artifact(m_viewPort.ToPx(TimePoint(350ms)), gdi::Artifact::Type::Flag, RGB(255, 0, 0)));
		}
		if (m_viewPort.Contains(TimePoint(400ms)))
		{
			Add(m_viewPort, *info, gdi::Artifact(m_viewPort.ToPx(TimePoint(400ms)), gdi::Artifact::Type::Flag, RGB(255, 0, 0), RGB(0, 255, 0)));
		}
		if (m_viewPort.Contains(TimePoint(500ms)))
		{
			Add(m_viewPort, *info, gdi::Artifact(m_viewPort.ToPx(TimePoint(500ms)), gdi::Artifact::Type::Flag));
		}

		auto sequence = std::make_shared<gdi::Line>(L"Move Sequence");
		sequence->Add(gdi::Artifact(615, gdi::Artifact::Type::Flag, RGB(160, 160, 170)));
		sequence->Add(gdi::Artifact(632, gdi::Artifact::Type::Flag, RGB(160, 160, 170)));
		sequence->Add(gdi::Artifact(636, gdi::Artifact::Type::Flag, RGB(255, 0, 0), RGB(0, 255, 0)));
		sequence->Add(gdi::Artifact(640, gdi::Artifact::Type::Flag, RGB(255, 0, 0)));

		auto data = std::make_shared<gdi::Line>(L"Arbitrary data");
		data->Add(gdi::Artifact(710, gdi::Artifact::Type::Flag, RGB(0, 0, 255)));
		data->Add(gdi::Artifact(721, gdi::Artifact::Type::Flag));
		data->Add(gdi::Artifact(722, gdi::Artifact::Type::Flag));

		gdi::TimeLines lines;
		lines.emplace_back(info);
		lines.emplace_back(sequence);
		lines.emplace_back(data);
		return lines;
	});

	m_timelineView.SetMouseScrollCallback([&](gdi::Pixel position, int direction) {
		if (direction > 0)
		{
			m_viewPort.ZoomOut(position);
		}
		else
		{
			m_viewPort.ZoomInTo(position);
		}
	});

	m_timelineView.Create(m_bottom, CWindow::rcDefault, gdi::CTimelineView::GetWndClassName(), WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | SS_OWNERDRAW);
	m_bottom.SetClient(m_timelineView);
	m_split.UpdateWindow();
	m_split.SetSplitterPosPct(75);
}

CLogViewTabItem2::~CLogViewTabItem2()
{
	m_split.DestroyWindow();
}


void CLogViewTabItem2::SetView(std::shared_ptr<CLogView> pView)
{
	m_pView = pView;
	m_top.SetClient(*m_pView);
}

CLogView& CLogViewTabItem2::GetView()
{
	return *m_pView;
}


} // namespace debugviewpp
} // namespace fusion
