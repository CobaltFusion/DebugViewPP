#pragma once

#include <Shellapi.h>

#include <atlapp.h>
#include <atlctrls.h>
#include <atlctrlx.h>

#include <atlbase.h> // Base ATL classes
#include <atlwin.h> // ATL windowing classes
#include <atlsplit.h>
#include <atlframe.h>
#include "CobaltFusion/dbgstream.h"
#include "CobaltFusion/Str.h"
#include "CobaltFusion/stringbuilder.h"
#include "Logview.h"
#include <iomanip>

#include <atlgdix.h> // CustomTabCtrl.h prerequisites
#include "atlcoll.h"
#include "CustomTabCtrl.h"
#include "DotNetTabCtrl.h"
#include "TabbedFrame.h"

#include "AtlWinExt.h"

#include "DebugView++Lib/TimelineDC.h"
#include "DebugView++Lib/CTimelineView.h"

namespace fusion {

inline std::wstring FormatUnits(int n, const std::wstring& unit)
{
	if (n == 0)
		return L"";
	if (n == 1)
		return wstringbuilder() << n << " " << unit;
	return wstringbuilder() << n << " " << unit << "s";
}

inline std::wstring FormatDuration(double seconds)
{
	int minutes = FloorTo<int>(seconds / 60);
	seconds -= 60 * minutes;

	int hours = minutes / 60;
	minutes -= 60 * hours;

	int days = hours / 24;
	hours -= 24 * days;

	if (days > 0)
		return wstringbuilder() << FormatUnits(days, L"day") << L" " << FormatUnits(hours, L"hour");

	if (hours > 0)
		return wstringbuilder() << FormatUnits(hours, L"hour") << L" " << FormatUnits(minutes, L"minute");

	if (minutes > 0)
		return wstringbuilder() << FormatUnits(minutes, L"minute") << L" " << FormatUnits(FloorTo<int>(seconds), L"second");

	static const wchar_t* units[] = {L"s", L"ms", L"µs", L"ns", nullptr};
	const wchar_t** unit = units;
	while (*unit != nullptr && seconds > 0 && seconds < 1)
	{
		seconds *= 1e3;
		++unit;
	}

	return wstringbuilder() << std::fixed << std::setprecision(3) << seconds << L" " << *unit;
}

template <typename T>
class TabItem : public CTabViewTabItem
{
public:
	T& GetView()
	{
		return m_logview;
	}

	gdi::CTimelineView& GetTimeLineView()
	{
		return m_timelineView;
	}

	void DisablePaneHeader(CPaneContainer& panecontainer)
	{
		panecontainer.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON, 0);
		panecontainer.m_cxyHeader = 0;
	}

	void Create(HWND parent)
	{
		auto client = RECT();
		GetClientRect(parent, &client);
		m_split.Create(parent, client);

		SetTabView(m_split);
		m_top.Create(m_split, L"");
		m_logview.Create(m_top, CWindow::rcDefault);
		m_top.SetClient(m_logview);
		fusion::AddDummyContent(m_logview);

		m_bottom.Create(m_split, L"");
		m_split.SetSplitterPanes(m_top, m_bottom, true);
		m_split.SetSplitterPos(560);

		DisablePaneHeader(m_top);
		DisablePaneHeader(m_bottom);
	}

private:
	T m_logview;
	CHorSplitterWindow m_split;
	CPaneContainer m_top;
	CPaneContainer m_bottom;
};

using CLogViewTabItem2 = TabItem<CLogView>;

} // namespace fusion
