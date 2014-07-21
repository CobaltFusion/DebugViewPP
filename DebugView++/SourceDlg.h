// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <vector>

#include "Resource.h"
#include "DebugView++Lib/LogSource.h"

namespace fusion {
namespace debugviewpp {

class CSourceDlg :
	public CDialogImpl<CSourceDlg>,
	public CDialogResize<CSourceDlg>
{
public:
	explicit CSourceDlg(const std::shared_ptr<LogSource>& pLogSource = nullptr);

	enum { IDD = IDD_SOURCE };

	BEGIN_DLGRESIZE_MAP(CSourceDlg)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnOk(UINT uNotifyCode, int nID, CWindow wndCtl);
	
	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID);
	void ExceptionHandler();

private:
	std::shared_ptr<LogSource> m_pLogSource;
};

} // namespace debugviewpp 
} // namespace fusion
