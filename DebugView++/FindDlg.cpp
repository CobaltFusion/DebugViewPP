// (C) Copyright Gert-Jan de Vos 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// See http://boosttestui.wordpress.com/ for the boosttestui home page.

#include "stdafx.h"
#include "resource.h"
#include "Utilities.h"
#include "MainFrm.h"
#include "FindDlg.h"

namespace gj {

CFindDlg::CFindDlg(CMainFrame& mainFrame) :
	m_mainFrame(mainFrame)
{
}

BOOL CFindDlg::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
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

void CFindDlg::OnNext(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/)
{
	m_mainFrame.FindNext(gj::GetDlgItemText(*this, IDC_TEXT));
}

void CFindDlg::OnPrevious(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/)
{
	m_mainFrame.FindPrevious(gj::GetDlgItemText(*this, IDC_TEXT));
}

void CFindDlg::OnClose(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/)
{
	ShowWindow(SW_HIDE);
}

} // namespace gj
