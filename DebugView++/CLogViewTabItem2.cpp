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

std::wstring FormatDuration(Duration d)
{
	if (d >= 1s) return wstringbuilder() << d.count() / 1e9 << L" s";
	if (d >= 1ms) return wstringbuilder() << d.count() / 1e6 << L" ms";
	if (d >= 1us) return wstringbuilder() << d.count() / 1e3 << L" us";
	return wstringbuilder() << d.count() << L" ns";
}

ViewPort::ViewPort(TimePoint begin, TimePoint end)
	: m_begin(begin)
	, m_end(end)
{
}

bool ViewPort::Contains(TimePoint p) const
{
	return (p >= m_begin) && (p <= m_end);
}

void Add(const ViewPort& viewport, gdi::Line& line, gdi::Artifact a)
{
	if (viewport.Contains(TimePoint(a.GetPosition()* 1ms)))
	{
		line.Add(a);
	}
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

	m_viewPort = ViewPort(TimePoint(0ms), TimePoint(1000ms));

	m_timelineView.SetFormatter([&](gdi::Pixel position) {
		return FormatDuration(position * 1ms);
	});

	m_timelineView.SetDataProvider([&](gdi::Pixel width, gdi::Pixel cursorPosition) {
		cdbg << " width = " << width << ", cursorPosition: " << cursorPosition << "\n";

		auto info = std::make_shared<gdi::Line>(L"Some info");
		Add(m_viewPort, *info, gdi::Artifact(300, gdi::Artifact::Type::Flag, RGB(255, 0, 0)));
		Add(m_viewPort, *info, gdi::Artifact(400, gdi::Artifact::Type::Flag, RGB(255, 0, 0), RGB(0, 255, 0)));
		Add(m_viewPort, *info, gdi::Artifact(500, gdi::Artifact::Type::Flag));

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
