// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "Resource.h"

namespace fusion {

class CMainFrame;

class CFindDlg :
	public CDialogImpl<CFindDlg>,
	public CDialogResize<CFindDlg>
{
public:
	explicit CFindDlg(CMainFrame& mainFrame);

	enum { IDD = IDD_FIND };

	BEGIN_MSG_MAP(CFindDlg)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
		COMMAND_ID_HANDLER_EX(IDOK, OnNext)
		COMMAND_ID_HANDLER_EX(IDC_NEXT, OnNext)
		COMMAND_ID_HANDLER_EX(IDC_PREVIOUS, OnPrevious)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnClose)
		COMMAND_ID_HANDLER_EX(IDCLOSE, OnClose)
		CHAIN_MSG_MAP(CDialogResize<CFindDlg>)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CFindDlg)
		DLGRESIZE_CONTROL(IDC_TEXT, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_NEXT, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_PREVIOUS, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDCLOSE, DLSZ_MOVE_X)
	END_DLGRESIZE_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnGetMinMaxInfo(MINMAXINFO* pInfo);
	void OnTextChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/);
	void OnPrevious(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/);
	void OnNext(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/);
	void OnClose(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/);

private:
	CMainFrame& m_mainFrame;
};

} // namespace fusion
