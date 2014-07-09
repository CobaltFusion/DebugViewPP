// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "Resource.h"

namespace fusion {
namespace debugviewpp {

class CRegExDlg :
	public CDialogImpl<CRegExDlg>
{
public:
	enum { IDD = IDD_REGEX };

private:
	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID);
	void ExceptionHandler();

	BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/);
	void OnClose(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/);
};

} // namespace debugviewpp 
} // namespace fusion
