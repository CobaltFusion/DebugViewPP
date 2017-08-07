// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <regex>
#include "CobaltFusion/stringbuilder.h"
#include "CobaltFusion/fusionassert.h"
#include "LogView.h"
#include <atlcrack.h>
#include <atlmisc.h>
#include <winbase.h>

namespace fusion {


BEGIN_MSG_MAP2(CLogView)
	MSG_WM_CREATE(OnCreate)
	MSG_WM_CLOSE(OnClose)
	MSG_WM_TIMER(OnTimer)

	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_ODSTATECHANGED, OnOdStateChanged)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_ODCACHEHINT, OnOdCacheHint)

	CHAIN_MSG_MAP_ALT(COwnerDraw<CLogView>, 1)
	//CHAIN_MSG_MAP(CDoubleBufferImpl<CLogView>) //DrMemory: GDI USAGE ERROR: DC 0x3e011cca that contains selected object being deleted
	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()


CLogView::CLogView()
{
}

void CLogView::OnException() const
{
}

void CLogView::OnException(const std::exception& ex)
{
}

void CLogView::DeleteItem(DELETEITEMSTRUCT* lParam)
{
    COwnerDraw<CLogView>::DeleteItem(lParam);
}

LRESULT CLogView::OnCreate(const CREATESTRUCT* /*pCreate*/)
{
	DefWindowProc();
	SetExtendedListViewStyle(GetWndExStyle(0));
	return 0;
}

void CLogView::SetFont(HFONT hFont)
{
    CListViewCtrl::SetFont(hFont);
    GetHeader().Invalidate();

    // Trigger WM_MEASUREPOS
    // See: http://www.codeproject.com/Articles/1401/Changing-Row-Height-in-an-owner-drawn-Control
    CRect rect;
    GetWindowRect(&rect);
    WINDOWPOS wp;
    wp.hwnd = *this;
    wp.cx = rect.Width();
    wp.cy = rect.Height();
    wp.flags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER;
    SendMessage(WM_WINDOWPOSCHANGED, 0, reinterpret_cast<LPARAM>(&wp));
}

void CLogView::OnClose()
{
}

void CLogView::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent != 1)
		return;

}

void CLogView::MeasureItem(MEASUREITEMSTRUCT* pMeasureItemStruct)
{
    OutputDebugStringA("CLogView::MeasureItem - debug");
	CClientDC dc(*this);

	Win32::GdiObjectSelection font(dc, GetFont());
	TEXTMETRIC metric;
	dc.GetTextMetrics(&metric);
	pMeasureItemStruct->itemHeight = metric.tmHeight;
}

void CLogView::DrawItem(DRAWITEMSTRUCT* pDrawItemStruct)
{
    OutputDebugStringA("CLogView::DrawItem - debug");

	DrawItem(pDrawItemStruct->hDC, pDrawItemStruct->itemID, pDrawItemStruct->itemState);
}

RECT CLogView::GetItemRect(int iItem, unsigned code) const
{
    RECT rect;
    CListViewCtrl::GetItemRect(iItem, &rect, code);
    return rect;
}

void CLogView::DrawItem(CDCHandle dc, int iItem, unsigned /*iItemState*/)
{
    SetMsgHandled(false);
}


LRESULT CLogView::OnOdStateChanged(NMHDR* pnmh)
{
	auto& nmhdr = *reinterpret_cast<NMLVODSTATECHANGE*>(pnmh);
	nmhdr;

	return 0;
}

LRESULT CLogView::OnOdCacheHint(NMHDR* pnmh)
{
	auto& nmhdr = *reinterpret_cast<NMLVCACHEHINT*>(pnmh);
	nmhdr;
	return 0;
}

void CLogView::DoPaint(CDCHandle dc)
{
	RECT rect;
	dc.GetClipBox(&rect);

    auto fill = RGB(10, 10, 255); //Colors::BackGround
	dc.FillSolidRect(&rect, fill);

	DefWindowProc(WM_PAINT, reinterpret_cast<WPARAM>(dc.m_hDC), 0);
}


} // namespace fusion
