// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <regex>
#include <vector>

#include "Grid.h"
#include "DebugView++Lib/FilterType.h"
#include "DebugView++Lib/Filter.h"
#include "Resource.h"

namespace fusion {
namespace debugviewpp {

class CFilterPageImpl :
	public CDialogImpl<CFilterPageImpl>,
	public CDialogResize<CFilterPageImpl>
{
public:
	CFilterPageImpl(const FilterType::type* filterTypes, size_t filterTypeCount, const MatchType::type* matchTypes, size_t matchTypeCount);

	std::vector<Filter> GetFilters() const;
	void SetFilters(const std::vector<Filter>& filters);

	enum { IDD = IDD_FILTER_PAGE };

	BEGIN_DLGRESIZE_MAP(CFilterPageImpl)
		DLGRESIZE_CONTROL(IDC_FILTER_GRID, DLSZ_SIZE_X | DLSZ_SIZE_Y)
	END_DLGRESIZE_MAP()

	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID);
	void ExceptionHandler();

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnDestroy();
	LRESULT OnDrag(NMHDR* phdr);
	void OnMouseMove(UINT nFlags, CPoint point);
	void OnLButtonUp(UINT nFlags, CPoint point);

private:
	void InsertFilter(int item, const Filter& filter);
	void AddFilter(const Filter& filter);
	std::wstring GetFilterText(int iItem) const;
	MatchType::type GetMatchType(int iItem) const;
	FilterType::type GetFilterType(int iItem) const;
	COLORREF GetFilterBgColor(int iItem) const;
	COLORREF GetFilterFgColor(int iItem) const;
	bool GetFilterEnable(int iItem) const;
	Filter GetFilter(int item) const;
	void UpdateGrid(int focus = 0);

	LRESULT OnAddItem(NMHDR* pnmh);
	LRESULT OnClickItem(NMHDR* pnmh);
	LRESULT OnItemChanged(NMHDR* pnmh);

	const FilterType::type* m_filterTypes;
	size_t m_filterTypeCount;
	const MatchType::type* m_matchTypes;
	size_t m_matchTypeCount;
	CPropertyGridCtrl m_grid;
	std::vector<Filter> m_filters;
	CImageList m_dragImage;
	int m_dragItem;
	std::unique_ptr<ScopedCursor> m_dragCursor;
};

class CFilterPage : public CFilterPageImpl
{
public:
	template <size_t FilterTypeCount, size_t MatchTypeCount>
	CFilterPage(const FilterType::type (&filterTypes)[FilterTypeCount], const MatchType::type (&matchTypes)[MatchTypeCount]) :
		CFilterPageImpl(filterTypes, FilterTypeCount, matchTypes, MatchTypeCount)
	{
	}
};

} // namespace debugviewpp 
} // namespace fusion
