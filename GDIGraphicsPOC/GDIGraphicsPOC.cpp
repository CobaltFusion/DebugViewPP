#include "stdafx.h"
#include "windows.h"
#include <string>

#define WIN32_LEAN_AND_MEAN

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

#pragma warning(push, 3)
#pragma warning(disable : 4838)


#include <atlgdix.h> // CustomTabCtrl.h prerequisites
#include "atlcoll.h"
#include "CustomTabCtrl.h"
#include "DotNetTabCtrl.h"
#include "TabbedFrame.h"

#include "AtlWinExt.h"
#pragma warning(pop)


#include "DebugView++Lib/TimelineDC.h"
#include "DebugView++Lib/CTimelineView.h"

namespace fusion {

std::wstring FormatUnits(int n, const std::wstring& unit)
{
	if (n == 0)
		return L"";
	if (n == 1)
		return wstringbuilder() << n << " " << unit;
	return wstringbuilder() << n << " " << unit << "s";
}

std::wstring FormatDuration(double seconds)
{
	return wstringbuilder() << seconds;
}

std::wstring FormatDuration2(double seconds)
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

class CLogViewTabItem2 : public CTabViewTabItem
{
public:
	CLogView& GetView();
	gdi::CTimelineView& GetTimeLineView() { return m_timelineView; }
	void Create(HWND parent);
    void InitTimeLine();
private:
	
	CLogView m_logview;
	CHorSplitterWindow m_split;
	CPaneContainer m_top;
	CPaneContainer m_bottom;
	gdi::CTimelineView m_timelineView;
};

void CLogViewTabItem2::InitTimeLine()
{
	m_timelineView.SetFormatter([](gdi::Location l) {
		return Str(FormatDuration(l));
	});

	m_timelineView.SetDataProvider([](gdi::Location, gdi::Location) {
		auto info = std::make_shared<gdi::Line>(L"Some info");
		info->Add(gdi::Artifact(650, gdi::Artifact::Type::Flag, RGB(255, 0, 0)));
		info->Add(gdi::Artifact(700, gdi::Artifact::Type::Flag, RGB(255, 0, 0), RGB(0, 255, 0)));
		info->Add(gdi::Artifact(750, gdi::Artifact::Type::Flag));
		info->Add(gdi::Artifact(800, gdi::Artifact::Type::Flag));
		info->Add(gdi::Artifact(850, gdi::Artifact::Type::Flag));
		info->Add(gdi::Artifact(992, gdi::Artifact::Type::Flag));

		auto sequence = std::make_shared<gdi::Line>(L"Move Sequence");
		sequence->Add(gdi::Artifact(615, gdi::Artifact::Type::Flag, RGB(160, 160, 170)));
		sequence->Add(gdi::Artifact(632, gdi::Artifact::Type::Flag, RGB(160, 160, 170)));
		sequence->Add(gdi::Artifact(636, gdi::Artifact::Type::Flag, RGB(255, 0, 0), RGB(0, 255, 0)));
		sequence->Add(gdi::Artifact(640, gdi::Artifact::Type::Flag, RGB(255, 0, 0)));

		auto data = std::make_shared<gdi::Line>(L"Arbitrary data");
		data->Add(gdi::Artifact(710, gdi::Artifact::Type::Flag, RGB(0, 0, 255)));
		info->Add(gdi::Artifact(701, gdi::Artifact::Type::Flag));

		gdi::TimeLines lines;
		lines.emplace_back(info);
		lines.emplace_back(sequence);
		lines.emplace_back(data);
		return lines;
	});
}

void CLogViewTabItem2::Create(HWND parent)
{
	auto client = RECT();
	GetClientRect(parent, &client);

	m_split.Create(parent, client);
	SetTabView(m_split);

	m_top.Create(m_split, L"top panel");
	GetClientRect(m_top, &client);

	m_logview.Create(m_top, client);
	m_top.SetClient(m_logview);
	m_top.SetTitle(L"top label");

	m_bottom.Create(m_split, L"bottom panel");
	m_split.SetSplitterPanes(m_top, m_bottom, true);
	m_split.SetSplitterPos(500);

	InitTimeLine();

	m_timelineView.Create(m_bottom, CWindow::rcDefault, gdi::CTimelineView::GetWndClassName(),
		WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | SS_OWNERDRAW);

	m_timelineView.SetView(0.0, 1000.0);
	m_bottom.SetClient(m_timelineView); //uncomment this line to start rendering m_timelineView (breaks because it needs to be configured)
}

CLogView& CLogViewTabItem2::GetView()
{
	return m_logview;
}

class CMainFrame : public CTabbedFrameImpl<CMainFrame, CDotNetTabCtrl<CLogViewTabItem2>>
{
public:
	DECLARE_WND_CLASS(_T("CMainFrame Class"))

	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MSG_WM_MOUSEWHEEL(OnMouseWheel)
		MSG_WM_SIZE(OnSize)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

	LRESULT OnSize(UINT nType, CSize Extent)
	{
		cdbg << "OnSize:: " << Extent.cx << ", " << Extent.cy << "\n";
		UpdateLayout();
		return 1;
	}

	class ScaleData
	{
	public:
		ScaleData()
		{
		}

		int startBase = 0;
		int startPower = 1;

		int endBase = 1;
		int endPower = -3;

		int GetStart()
		{
			return startBase;
		}

		int GetEnd()
		{
			return endBase;
		}

		std::string GetUnit()
		{
			return "ms";
		}
	};

	BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
	{
		//if (zDelta > 0)
		//{
		//	m_timelineView.Zoom(2.0);
		//}
		//else
		//{
		//	m_timelineView.Zoom(0.5);
		//}
		return TRUE;
	}

	void DisablePaneHeader(CPaneContainer& panecontainer)
	{
		panecontainer.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON, 0);
		panecontainer.m_cxyHeader = 0;
	}

	void AddTab(const std::wstring name)
	{
		auto lvi = std::make_shared<CLogViewTabItem2>();
		m_tabitems.push_back(lvi);
		lvi->Create(*this);
		lvi->SetText(name.c_str());

		int newIndex = GetTabCtrl().GetItemCount();
		GetTabCtrl().InsertItem(newIndex, lvi.get());
		GetTabCtrl().SetCurSel(newIndex);
	}

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		auto rect = RECT();
		GetClientRect(&rect);

		// block 1
		//auto lvi = std::make_shared<CLogViewTabItem2>();
		//m_tabitems.push_back(lvi);
		//auto& timeline = lvi->GetTimeLineView();
  //      lvi->InitTimeLine();
  //      timeline.Create(*this, CWindow::rcDefault, gdi::CTimelineView::GetWndClassName(),
  //          WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | SS_OWNERDRAW);

  //      timeline.SetView(0.0, 1000.0);

		// block 2
		CreateTabWindow(*this, rect, CTCS_CLOSEBUTTON | CTCS_DRAGREARRANGE);
		AddTab(L"Tab1");
		AddTab(L"Tab2");

		ShowTabControl();
		return 0;
	}

	LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		DestroyWindow();
		return 0;
	}

	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		PostQuitMessage(0);
		return 0;
	}
	std::vector<std::shared_ptr<CLogViewTabItem2>> m_tabitems;
	CLogView m_logview;
};

} // namespace fusion


CComModule _Module;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hInstPrev,
	LPSTR szCmdLine, int nCmdShow)
{
	fusion::CMainFrame wndMain;
	MSG msg;

	// Create & show our main window
	if (NULL == wndMain.Create(NULL, CWindow::rcDefault, _T("WTL Frame")))
	{
		// Bad news, window creation failed
		return 1;
	}

	wndMain.ShowWindow(nCmdShow);
	wndMain.UpdateWindow();

	// Run the message loop
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}
