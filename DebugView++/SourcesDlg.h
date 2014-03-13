// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <vector>

#include "SourcesPage.h"
#include "PropertyColorItem.h"
#include "Resource.h"
#include "Filter.h"

namespace fusion {
namespace debugviewpp {

class CSourcesDlg :
	public CDialogImpl<CSourcesDlg>,
	public CDialogResize<CSourcesDlg>
{
public:
	CSourcesDlg();
	
	enum { IDD = IDD_SOURCES };

	BEGIN_DLGRESIZE_MAP(CSourcesDlg)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnOk(UINT uNotifyCode, int nID, CWindow wndCtl);

	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID);
	void ExceptionHandler();

private:
	CTabCtrl m_tabCtrl;
	CSourcesPageImpl m_sourcesPage;

	SIZE m_border;
};

} // namespace debugviewpp 
} // namespace fusion
