// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <regex>
#include <vector>

#include "Grid.h"
#include "Resource.h"
#include "Utilities.h"
#include "Types.h"

namespace fusion {

struct MessageFilter
{
	MessageFilter();
	MessageFilter(const std::string& text, FilterType::type type, COLORREF bgColor, COLORREF fgColor, bool enable);

	std::string text;
	std::regex re;
	FilterType::type type;
	COLORREF bgColor;
	COLORREF fgColor;
	bool enable;
};

class CMessageFilterPage :
	public CDialogImpl<CMessageFilterPage>,
	public CDialogResize<CMessageFilterPage>
{
public:
	explicit CMessageFilterPage(const std::vector<MessageFilter>& filters);

	std::vector<MessageFilter> GetFilters() const;
	void SetFilters(const std::vector<MessageFilter>& filters);

	enum { IDD = IDD_FILTER_PAGE };

	BEGIN_DLGRESIZE_MAP(CMessageFilterPage)
		DLGRESIZE_CONTROL(IDC_GRID, DLSZ_SIZE_X | DLSZ_SIZE_Y)
	END_DLGRESIZE_MAP()

	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID);
	void ExceptionHandler();

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnDestroy();

private:
	void AddFilter(const MessageFilter& filter);
	std::wstring GetFilterText(int iItem) const;
	FilterType::type GetFilterType(int iItem) const;
	COLORREF GetFilterBgColor(int iItem) const;
	COLORREF GetFilterFgColor(int iItem) const;
	bool GetFilterEnable(int iItem) const;

	LRESULT OnAddItem(NMHDR* pnmh);
	LRESULT OnClickItem(NMHDR* pnmh);
	LRESULT OnItemChanged(NMHDR* pnmh);

	CPropertyGridCtrl m_grid;
	std::vector<MessageFilter> m_filters;
};

} // namespace fusion
