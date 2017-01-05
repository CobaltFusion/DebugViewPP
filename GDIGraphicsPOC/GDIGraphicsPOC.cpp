#include "stdafx.h"
#include "windows.h"
#include <string>

#include "DebugView++Lib/TimelineDC.h"
#include "DebugView++Lib/CTimelineView.h"

#define WIN32_LEAN_AND_MEAN

#include <Shellapi.h>

#include <atlctrls.h>
#include <atlctrlw.h>
#include <atlctrlx.h>

#include <atlbase.h>        // Base ATL classes
#include <atlwin.h>         // ATL windowing classes
#include <atlsplit.h>
#include <atlframe.h>
#include "CobaltFusion/dbgstream.h"

namespace fusion
{

class CMainFrame : 
	public CFrameWindowImpl<CMainFrame>
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
		if (zDelta > 0)
		{
			m_timelineView.Zoom(1.01);
		}
		else
		{
			m_timelineView.Zoom(0.99);
		}
		return TRUE;
	}

	void DisablePaneHeader(CPaneContainer& panecontainer)
	{
		panecontainer.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON, 0);
		panecontainer.m_cxyHeader = 0;
	}

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		m_zoomFactor = 1.0;
		RECT rect = RECT();
		GetClientRect(&rect);
		m_split.Create(*this, rect, NULL, 0, WS_EX_CLIENTEDGE);
		m_hWndClient = m_split;

		m_top.Create(m_split, L"Top Pane");
		m_bottom.Create(m_split, L"");
		DisablePaneHeader(m_bottom);
		m_split.SetSplitterPanes(m_top, m_bottom, true);
		m_split.SetSplitterPos(600);
		//m_split.SetSinglePaneMode(SPLIT_PANE_TOP);  // this hides the bottom pane and the splitter

		m_timelineView.Create(m_bottom, rcDefault, gdi::CTimelineView::GetWndClassName(),
			WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | SS_OWNERDRAW);

		// start, end, exponent
		m_timelineView.SetView(0, 1000, -3);
		auto info = m_timelineView.Add("Some info");
		info->Add(gdi::Artifact(650, gdi::Artifact::Type::Flag, RGB(255, 0, 0)));
		info->Add(gdi::Artifact(700, gdi::Artifact::Type::Flag, RGB(255, 0, 0), RGB(0, 255, 0)));
		info->Add(gdi::Artifact(750, gdi::Artifact::Type::Flag));
		info->Add(gdi::Artifact(800, gdi::Artifact::Type::Flag));
		info->Add(gdi::Artifact(850, gdi::Artifact::Type::Flag));
		info->Add(gdi::Artifact(992, gdi::Artifact::Type::Flag));

		auto sequence = m_timelineView.Add("Move Sequence");
		sequence->Add(gdi::Artifact(615, gdi::Artifact::Type::Flag, RGB(160, 160, 170)));
		sequence->Add(gdi::Artifact(632, gdi::Artifact::Type::Flag, RGB(160, 160, 170)));
		sequence->Add(gdi::Artifact(636, gdi::Artifact::Type::Flag, RGB(255, 0, 0), RGB(0, 255, 0)));
		sequence->Add(gdi::Artifact(640, gdi::Artifact::Type::Flag, RGB(255, 0, 0)));

		auto data = m_timelineView.Add("Arbitrary data");
		data->Add(gdi::Artifact(710, gdi::Artifact::Type::Flag, RGB(0, 0, 255)));


		info->Add(gdi::Artifact(701, gdi::Artifact::Type::Flag));		// todo: adding this line crashes the app, m_timelineView.Add invalidates the returned &? find out why

		m_bottom.SetClient(m_timelineView);
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

	CHorSplitterWindow m_split;
	CPaneContainer m_top;
	CPaneContainer m_bottom;
	gdi::CTimelineView m_timelineView;
	double m_zoomFactor;

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
