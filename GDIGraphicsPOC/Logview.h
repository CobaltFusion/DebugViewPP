// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <vector>
#include <deque>


#include "atlapp.h"
#include "atlgdi.h"
#include "atlframe.h"
#include "atlctrls.h"
#include "AtlWinExt.h"
#include <atlcrack.h>


namespace fusion {

template <typename T>
void AddDummyContent(T& t)
{
	t.InsertColumn(0, _T("Scoobies2"), LVCFMT_LEFT, -1, -1);
	t.InsertItem(0, _T("01 Willow is een test regel met een hoop tekens er achter"));
	t.InsertItem(1, _T("02 Buffy is een test regel met een hoop tekens er achter"));
	t.InsertItem(2, _T("03 Giles is een test regel met een hoop tekens er achter"));
	t.InsertItem(3, _T("04 Willow is een test regel met een hoop tekens er achter"));
	t.InsertItem(4, _T("05 Buffy is een test regel met een hoop tekens er achter"));
	t.InsertItem(5, _T("06 Giles is een test regel met een hoop tekens er achter"));
	t.InsertItem(6, _T("07 Willow is een test regel met een hoop tekens er achter"));
	t.InsertItem(7, _T("08 Buffy is een test regel met een hoop tekens er achter"));
	t.InsertItem(8, _T("09 Giles is een test regel met een hoop tekens er achter"));
	t.InsertItem(9, _T("10 Giles is een test regel met een hoop tekens er achter"));
	t.InsertItem(10, _T("11 Willow is een test regel met een hoop tekens er achter"));
	t.InsertItem(11, _T("12 Buffy is een test regel met een hoop tekens er achter"));
	t.InsertItem(12, _T("13 Giles is een test regel met een hoop tekens er achter"));
	t.InsertItem(13, _T("14 Willow is een test regel met een hoop tekens er achter"));
	t.InsertItem(14, _T("15 Buffy is een test regel met een hoop tekens er achter"));
	t.InsertItem(15, _T("16 Giles is een test regel met een hoop tekens er achter"));
	t.InsertItem(16, _T("17 Willow is een test regel met een hoop tekens er achter"));
	t.InsertItem(17, _T("18 Buffy is een test regel met een hoop tekens er achter"));
	t.InsertItem(18, _T("19 Giles is een test regel met een hoop tekens er achter"));
	t.InsertItem(19, _T("20 Giles is een test regel met een hoop tekens er achter"));
	t.InsertItem(20, _T("21 Willow is een test regel met een hoop tekens er achter"));
	t.InsertItem(21, _T("22 Buffy is een test regel met een hoop tekens er achter"));
	t.InsertItem(22, _T("23 Giles is een test regel met een hoop tekens er achter"));
	t.InsertItem(23, _T("24 Willow is een test regel met een hoop tekens er achter"));
	t.InsertItem(24, _T("25 Buffy is een test regel met een hoop tekens er achter"));
	t.InsertItem(25, _T("26 Giles is een test regel met een hoop tekens er achter"));
	t.InsertItem(26, _T("27 Willow is een test regel met een hoop tekens er achter"));
	t.InsertItem(27, _T("28 Buffy is een test regel met een hoop tekens er achter"));
	t.InsertItem(28, _T("29 Giles is een test regel met een hoop tekens er achter"));
	t.InsertItem(29, _T("30 Giles is een test regel met een hoop tekens er achter"));
	t.InsertItem(30, _T("31 Willow is een test regel met een hoop tekens er achter"));
	t.InsertItem(31, _T("32 Buffy is een test regel met een hoop tekens er achter"));
	t.InsertItem(32, _T("33 Giles is een test regel met een hoop tekens er achter"));
	t.InsertItem(33, _T("34 Willow is een test regel met een hoop tekens er achter"));
	t.InsertItem(34, _T("35 Buffy is een test regel met een hoop tekens er achter"));
	t.InsertItem(35, _T("36 Giles is een test regel met een hoop tekens er achter"));
	t.InsertItem(36, _T("37 Willow is een test regel met een hoop tekens er achter"));
	t.InsertItem(37, _T("38 Buffy is een test regel met een hoop tekens er achter"));
	t.InsertItem(38, _T("39 Giles is een test regel met een hoop tekens er achter"));
	t.InsertItem(39, _T("40 Giles is een test regel met een hoop tekens er achter"));
	t.SetColumnWidth(0, 800);

	HFONT font = CreateFont(-10, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, L"Courier");
	t.SetFont(font);
}

class CLogView : public CWindowImpl<CLogView, CListViewCtrl,
					 CWinTraitsOR<
						 LVS_OWNERDRAWFIXED | LVS_REPORT | LVS_OWNERDATA | LVS_NOSORTHEADER | LVS_SHOWSELALWAYS,
						 LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_HEADERDRAGDROP>>,
				 public WTL::COwnerDraw<CLogView>
{
public:
	CLogView();

	DECLARE_WND_SUPERCLASS(nullptr, CListViewCtrl::GetWndClassName())

	BEGIN_MSG_MAP(CLogView)
		MSG_WM_CLOSE(OnClose)
		CHAIN_MSG_MAP_ALT(COwnerDraw<CLogView>, 1)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

	void Create(HWND hWndParent, ATL::_U_RECT rect = NULL);
	void DeleteItem(DELETEITEMSTRUCT* lParam);
	void MeasureItem(MEASUREITEMSTRUCT* pMeasureItemStruct);
	void DrawItem(DRAWITEMSTRUCT* pDrawItemStruct);
	void SetFont(HFONT hFont);

private:
	void OnClose();
	RECT GetItemRect(int iItem, unsigned code) const;
	void DrawItem(CDCHandle dc, int iItem, unsigned iItemState);
};

} // namespace fusion
