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


namespace fusion {

namespace Colors {
    const COLORREF Auto = 0x80808080;
    const COLORREF BackGround = GetSysColor(COLOR_WINDOW);
    const COLORREF Text = GetSysColor(COLOR_WINDOWTEXT);
    const COLORREF Highlight = RGB(255, 255, 55);
    const COLORREF Selection = RGB(128, 255, 255);
    const COLORREF ItemHighlight = GetSysColor(COLOR_HIGHLIGHT);
    const COLORREF ItemHighlightText = GetSysColor(COLOR_HIGHLIGHTTEXT);
} // namespace Colors

struct Column
{
	enum type
	{
		Bookmark = 0,
		Line,
		Date,
		Time,
		Pid,
		Process,
		Message,
		Count
	};
};


struct ColumnInfo
{
	bool enable;
	LVCOLUMN column;
};

class CLogView : public WTL::CDoubleBufferWindowImpl<CLogView, CListViewCtrl,
					 CWinTraitsOR<
						 LVS_OWNERDRAWFIXED | LVS_REPORT | LVS_OWNERDATA | LVS_NOSORTHEADER | LVS_SHOWSELALWAYS,
						 LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_HEADERDRAGDROP>>,
				 public WTL::COwnerDraw<CLogView>,
                 ExceptionHandler<CLogView, std::exception>
{
public:
    CLogView();

	DECLARE_WND_SUPERCLASS(nullptr, CListViewCtrl::GetWndClassName())

	void DoPaint(WTL::CDCHandle dc);
    void DeleteItem(DELETEITEMSTRUCT* lParam);


	void MeasureItem(MEASUREITEMSTRUCT* pMeasureItemStruct) const;
	void DrawItem(DRAWITEMSTRUCT* pDrawItemStruct) const;

private:
	DECLARE_MSG_MAP()

	void OnException() const;
	void OnException(const std::exception& ex);
	LRESULT OnCreate(const CREATESTRUCT* pCreate);
	void OnClose();
	void OnTimer(UINT_PTR nIDEvent);

	LRESULT OnOdStateChanged(NMHDR* pnmh);
	LRESULT OnOdCacheHint(NMHDR* pnmh);
    RECT GetItemRect(int iItem, unsigned code) const;

	Column::type SubItemToColumn(int iSubItem) const;

	void DrawItem(CDCHandle dc, int iItem, unsigned iItemState) const;


};

} // namespace fusion
