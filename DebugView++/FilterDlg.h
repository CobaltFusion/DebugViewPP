//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://boosttestui.wordpress.com/ for the boosttestui home page.

#pragma once

#include <vector>
#include "dbgstream.h"
#include "PropertyGrid.h"
#include "ColorCombo.h"
#include "Resource.h"

namespace gj {

class CFilterDlg :
	public CDialogImpl<CFilterDlg>,
	public CDialogResize<CFilterDlg>
{
public:
	explicit CFilterDlg(const std::wstring& name);
	CFilterDlg(const std::wstring& name, const std::vector<std::string>& filters);

	std::wstring GetName() const;
	std::vector<std::string> GetFilters() const;

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
	std::unique_ptr<CColorPickerListCtrl> CreateColorCtrl();
	void AddFilter(const std::wstring& filter);
	void RemoveFilter(int index);

	CPropertyGridCtrl m_grid;
	std::vector<std::unique_ptr<CColorPickerListCtrl>> m_colorCtrls;
	std::wstring m_name;
	std::vector<std::string> m_filters;
};

} // namespace gj
