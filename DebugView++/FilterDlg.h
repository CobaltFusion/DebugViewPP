// (C) Copyright Gert-Jan de Vos 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// See http://boosttestui.wordpress.com/ for the boosttestui home page.

#pragma once

#include <vector>
#include <regex>
#include "PropertyGrid.h"
#include "Resource.h"

namespace gj {

struct FilterType
{
	enum type
	{
		Include,
		Exclude,
		Highlight
	};
};

struct LogFilter
{
	LogFilter();
	LogFilter(const std::string& text, FilterType::type type, COLORREF bgColor, COLORREF fgColor, bool enable);

	std::string text;
	std::regex re;
	FilterType::type type;
	COLORREF bgColor;
	COLORREF fgColor;
	bool enable;
};

class CFilterDlg :
	public CDialogImpl<CFilterDlg>,
	public CDialogResize<CFilterDlg>
{
public:
	explicit CFilterDlg(const std::wstring& name);
	CFilterDlg(const std::wstring& name, const std::vector<LogFilter>& filters);

	std::wstring GetName() const;
	std::vector<LogFilter> GetFilters() const;

	enum { IDD = IDD_FILTER };

	BEGIN_DLGRESIZE_MAP(CFilterDlg)
		DLGRESIZE_CONTROL(IDC_GRID, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnDestroy();

	void OnAdd(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnDelete(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnOk(UINT uNotifyCode, int nID, CWindow wndCtl);
	LRESULT OnAddItem(NMHDR* pnmh);
	LRESULT OnClickItem(NMHDR* pnmh);

	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID);
	void ExceptionHandler();

private:
	void AddFilter(const LogFilter& filter);
	std::wstring GetFilterText(int iItem) const;
	FilterType::type GetFilterType(int iItem) const;
	COLORREF GetFilterBgColor(int iItem) const;
	COLORREF GetFilterFgColor(int iItem) const;
	bool GetFilterEnable(int iItem) const;

	CPropertyGridCtrl m_grid;
	std::wstring m_name;
	std::vector<LogFilter> m_filters;
};

} // namespace gj
