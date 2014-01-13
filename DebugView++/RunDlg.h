// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include "Resource.h"

namespace fusion {
namespace debugviewpp {

class CRunDlg :
	public CDialogImpl<CRunDlg>,
	public CDialogResize<CRunDlg>
{
public:
	enum { IDD = IDD_RUN };

	std::wstring GetPathName() const;
	std::wstring GetArguments() const;

	BEGIN_DLGRESIZE_MAP(CRunDlg)
		DLGRESIZE_CONTROL(IDC_RUN, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_ARGUMENTS, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_BROWSE, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

private:
	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID);
	void ExceptionHandler();

	BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/);
	void OnBrowse(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnCancel(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/);
	void OnOk(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/);

	std::wstring m_pathName;
	std::wstring m_arguments;
};

} // namespace debugviewpp 
} // namespace fusion
