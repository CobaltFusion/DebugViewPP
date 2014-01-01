// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <regex>

#include "Grid.h"
#include "FilterType.h"
#include "Resource.h"
#include "Utilities.h"

namespace fusion {
namespace debugviewpp {

struct ProcessFilter
{
	ProcessFilter();
	ProcessFilter(const std::string& text, DWORD pid, FilterType::type type, COLORREF bgColor = RGB(255, 255, 255), COLORREF fgColor = RGB(0, 0, 0), bool enable = true);

	std::string text;
	std::regex re;
	DWORD pid;
	FilterType::type type;
	COLORREF bgColor;
	COLORREF fgColor;
	bool enable;
};

class CProcessFilterPage :
	public CDialogImpl<CProcessFilterPage>,
	public CDialogResize<CProcessFilterPage>
{
public:
	explicit CProcessFilterPage(const std::vector<ProcessFilter>& filters);

	std::vector<ProcessFilter> GetFilters() const;
	void SetFilters(const std::vector<ProcessFilter>& filters);

	enum { IDD = IDD_FILTER_PAGE };

	BEGIN_DLGRESIZE_MAP(CProcessFilterPage)
		DLGRESIZE_CONTROL(IDC_GRID, DLSZ_SIZE_X | DLSZ_SIZE_Y)
	END_DLGRESIZE_MAP()

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnDestroy();

	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID);
	void ExceptionHandler();

private:
	void AddFilter(const ProcessFilter& filter);
	std::wstring GetFilterText(int iItem) const;
	FilterType::type GetFilterType(int iItem) const;
	COLORREF GetFilterBgColor(int iItem) const;
	COLORREF GetFilterFgColor(int iItem) const;
	bool GetFilterEnable(int iItem) const;

	LRESULT OnAddItem(NMHDR* pnmh);
	LRESULT OnClickItem(NMHDR* pnmh);
	LRESULT OnItemChanged(NMHDR* pnmh);

	CPropertyGridCtrl m_grid;
	std::vector<ProcessFilter> m_filters;
};

} // namespace debugviewpp 
} // namespace fusion
