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
#include "atlscrl.h"
#include "Win32/gdi.h"

namespace fusion {
namespace gdi {

class Window
{
public:
	Window(HINSTANCE hInstance, WNDPROC messageHandler, const std::wstring& uniqueClassName, const std::wstring& title, int width, int height);
	void Show(int nCmdShow);

private:
	WNDCLASSEX wc;
	HWND hwnd;
};

class MessageLoop
{
public:
	int run();
private:
	MSG msg;
};


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


class CTimelineView :
	public CDoubleBufferWindowImpl<CTimelineView, ATL::CWindow,
	CWinTraitsOR<
	LVS_OWNERDRAWFIXED | LVS_REPORT | LVS_OWNERDATA | LVS_NOSORTHEADER | LVS_SHOWSELALWAYS,
	LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_HEADERDRAGDROP>>,
	public COwnerDraw<CTimelineView>
	//public ExceptionHandler<CTimelineView, std::exception>
{
public:
	BEGIN_MSG_MAP(CPaintBkgnd)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
	END_MSG_MAP()

	LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		return 1;    // we painted the background
	}

	void DoPaint(CDCHandle cdc)
	{
		graphics::DeviceContextEx dc(cdc.m_hDC);

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

};

class Timeline
{
	// zoomable->conceptually new, the size of the view can become as large as the entire buffer.
	//  panneble->horizonal scrolling : similar to page - up / down in existing view

};


} // namespace gdi
} // namespace fusion
