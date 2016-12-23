#include "stdafx.h"
#include "windows.h"
#include <string>

#include "Win32/gdi.h"
#include "gdi.h"

#define WIN32_LEAN_AND_MEAN


#include <Shellapi.h>

#include <atlctrls.h>
#include <atlctrlw.h>
#include <atlctrlx.h>

#include <atlbase.h>        // Base ATL classes
#include <atlwin.h>         // ATL windowing classes
#include <atlsplit.h>
#include <atlframe.h>

namespace fusion
{

void DoPaint(graphics::DeviceContextEx dc)
{
	int y = 60;

	auto grey = RGB(160, 160, 170);
	dc.DrawTimeline(L"Move Sequence", 15, y, 500, grey);
	dc.DrawFlag(L"tag", 200, y);
	dc.DrawFlag(L"tag", 250, y);
	dc.DrawSolidFlag(L"tag", 260, y, RGB(255, 0, 0), RGB(0, 255, 0));
	dc.DrawFlag(L"tag", 270, y);

	dc.DrawTimeline(L"Arbitrary data", 15, 90, 500, grey);
	dc.DrawFlag(L"blueFlag", 470, 90, RGB(0, 0, 255), true);
}

PAINTSTRUCT ps;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
		//SetTimer(hwnd, 10, 250, NULL);
		break;

	case WM_PAINT:
		BeginPaint(hwnd, &ps);
		DoPaint(graphics::DeviceContextEx(GetWindowDC(hwnd)));
		EndPaint(hwnd, &ps);
		break;

	case WM_TIMER:
		InvalidateRect(hwnd, NULL, TRUE);
		break;

	case WM_DESTROY:
		PostQuitMessage(WM_QUIT);
		break;

	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}

} // namespace fusion


template <class T, COLORREF t_crBrushColor>
class CPaintBkgnd
{
public:
	CPaintBkgnd() { m_hbrBkgnd = CreateSolidBrush(t_crBrushColor); }
	~CPaintBkgnd() { DeleteObject(m_hbrBkgnd); }

	BEGIN_MSG_MAP(CPaintBkgnd)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
	END_MSG_MAP()

	LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		T*   pT = static_cast<T*>(this);
		HDC  dc = (HDC)wParam;
		RECT rcClient;

		pT->GetClientRect(&rcClient);
		FillRect(dc, &rcClient, m_hbrBkgnd);
		return 1;    // we painted the background
	}

protected:
	HBRUSH m_hbrBkgnd;
};

class CMainFrame : public CFrameWindowImpl<CMainFrame>, public CPaintBkgnd<CMainFrame, RGB(0, 0, 255)>
{
public:
	DECLARE_WND_CLASS(_T("My Window Class"))

	typedef CPaintBkgnd<CMainFrame, RGB(0, 0, 255)> CPaintBkgndBase;

	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		CHAIN_MSG_MAP(CPaintBkgndBase)
	END_MSG_MAP()


	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		m_split.Create(*this, rcDefault);

		// Set the splitter as the client area window, and resize
		// the splitter to match the frame size.
		m_hWndClient = m_split;
		//m_split.SetSinglePaneMode(SPLIT_PANE_NONE);
		m_top.Create(m_split, L"Top Pane");
		m_bottom.Create(m_split, L"Bottom Pane");

		m_split.SetSplitterPanes(m_top, m_bottom, true);
		UpdateLayout();
		m_split.SetSplitterPos(600);

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

};


CComModule _Module;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hInstPrev,
	LPSTR szCmdLine, int nCmdShow)
{
	CMainFrame wndMain;
	MSG msg;

	// Create & show our main window
	if (NULL == wndMain.Create(NULL, CWindow::rcDefault,
		_T("My First ATL Window")))
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


//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
//{
//	using namespace fusion::graphics;
//
//	Window window(hInstance, fusion::WndProc, L"MyClass", L"My window title", 1000, 500);
//	window.Show(nCmdShow);
//
//	MessageLoop messageLoop;
//	return messageLoop.run();
//}
