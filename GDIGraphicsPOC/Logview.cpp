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

namespace fusion {


BEGIN_MSG_MAP2(CLogView)
	MSG_WM_CREATE(OnCreate)
	MSG_WM_CLOSE(OnClose)
	MSG_WM_TIMER(OnTimer)

	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_ODSTATECHANGED, OnOdStateChanged)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_ODCACHEHINT, OnOdCacheHint)

	CHAIN_MSG_MAP_ALT(COwnerDraw<CLogView>, 1)
	CHAIN_MSG_MAP(CDoubleBufferImpl<CLogView>) //DrMemory: GDI USAGE ERROR: DC 0x3e011cca that contains selected object being deleted
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

Column::type CLogView::SubItemToColumn(int iSubItem) const
{
	LVCOLUMN column;
	column.mask = LVCF_SUBITEM;
	GetColumn(iSubItem, &column);
	return static_cast<Column::type>(column.iSubItem);
}

LRESULT CLogView::OnCreate(const CREATESTRUCT* /*pCreate*/)
{
	DefWindowProc();
	SetExtendedListViewStyle(GetWndExStyle(0));

    SetItemCountEx(10, LVSICF_NOSCROLL);
    SetItemState(5, LVIS_FOCUSED, LVIS_FOCUSED);
	return 0;
}

void CLogView::OnClose()
{
}

void CLogView::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent != 1)
		return;

}

void CLogView::MeasureItem(MEASUREITEMSTRUCT* pMeasureItemStruct) const
{
	CClientDC dc(*this);

	Win32::GdiObjectSelection font(dc, GetFont());
	TEXTMETRIC metric;
	dc.GetTextMetrics(&metric);
	pMeasureItemStruct->itemHeight = metric.tmHeight;
}

void CLogView::DrawItem(DRAWITEMSTRUCT* pDrawItemStruct) const
{
	DrawItem(pDrawItemStruct->hDC, pDrawItemStruct->itemID, pDrawItemStruct->itemState);
}

RECT CLogView::GetItemRect(int iItem, unsigned code) const
{
    RECT rect;
    CListViewCtrl::GetItemRect(iItem, &rect, code);
    return rect;
}

void CLogView::DrawItem(CDCHandle dc, int iItem, unsigned /*iItemState*/) const
{
	auto rect = GetItemRect(iItem, LVIR_BOUNDS);
	//auto data = GetItemData(iItem);

	bool selected = GetItemState(iItem, LVIS_SELECTED) == LVIS_SELECTED;
	bool focused = GetItemState(iItem, LVIS_FOCUSED) == LVIS_FOCUSED;
    auto bkColor = selected ? Colors::ItemHighlight : Colors::BackGround;
    auto txColor = selected ? Colors::ItemHighlightText : Colors::Text;

	rect.left += GetColumnWidth(0);
	dc.FillSolidRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, bkColor);

	Win32::ScopedBkColor bcol(dc, bkColor);
	Win32::ScopedTextColor tcol(dc, txColor);
    if (focused)
	    dc.DrawFocusRect(&rect);
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
