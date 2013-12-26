// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "resource.h"
#include "Utilities.h"
#include "MainFrame.h"
#include "FindDlg.h"

namespace fusion {

CFindDlg::CFindDlg(CMainFrame& mainFrame) :
	m_mainFrame(mainFrame)
{
}

BOOL CFindDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
	CenterWindow(GetParent());
	DlgResize_Init();
	return TRUE;
}

void CFindDlg::OnGetMinMaxInfo(MINMAXINFO* pInfo)
{
	RECT rect;
	GetWindowRect(&rect);
	pInfo->ptMinTrackSize.x = 300;
	pInfo->ptMinTrackSize.y = pInfo->ptMaxTrackSize.y = rect.bottom - rect.top;
}

void CFindDlg::OnNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/)
{
	m_mainFrame.FindNext(fusion::GetDlgItemText(*this, IDC_TEXT));
}

void CFindDlg::OnPrevious(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/)
{
	m_mainFrame.FindPrevious(fusion::GetDlgItemText(*this, IDC_TEXT));
}

void CFindDlg::OnClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/)
{
	ShowWindow(SW_HIDE);
}

void CFindDlg::Show()
{
	ShowWindow(SW_SHOW);
	GetDlgItem(IDC_TEXT).SetFocus();
}

} // namespace fusion
