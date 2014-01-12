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
#include "MatchType.h"
#include "FilterType.h"

namespace fusion {
namespace debugviewpp {

class CFilterPage :
	public CDialogImpl<CFilterPage>,
	public CDialogResize<CFilterPage>
{
public:
	CFilterPage(const FilterType::type* filterTypes, size_t filterTypeCount);

	std::vector<Filter> GetFilters() const;
	void SetFilters(const std::vector<Filter>& filters);

	enum { IDD = IDD_FILTER_PAGE };

	BEGIN_DLGRESIZE_MAP(CFilterPage)
		DLGRESIZE_CONTROL(IDC_GRID, DLSZ_SIZE_X | DLSZ_SIZE_Y)
	END_DLGRESIZE_MAP()

	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID);
	void ExceptionHandler();

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnDestroy();

private:
	void AddFilter(const Filter& filter);
	std::wstring GetFilterText(int iItem) const;
	MatchType::type GetMatchType(int iItem) const;
	FilterType::type GetFilterType(int iItem) const;
	COLORREF GetFilterBgColor(int iItem) const;
	COLORREF GetFilterFgColor(int iItem) const;
	bool GetFilterEnable(int iItem) const;

	LRESULT OnAddItem(NMHDR* pnmh);
	LRESULT OnClickItem(NMHDR* pnmh);
	LRESULT OnItemChanged(NMHDR* pnmh);

	const FilterType::type* m_filterTypes;
	size_t m_filterTypeCount;
	CPropertyGridCtrl m_grid;
	std::vector<Filter> m_filters;
};

} // namespace debugviewpp 
} // namespace fusion
