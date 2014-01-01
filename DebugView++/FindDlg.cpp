// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "dbgstream.h"
#include "resource.h"
#include "Utilities.h"
#include "MainFrame.h"
#include "FindDlg.h"

namespace fusion {
namespace debugviewpp {

CFindDlg::CFindDlg(CMainFrame& mainFrame) :
	m_mainFrame(mainFrame)
{
}

BOOL CFindDlg::PreTranslateMessage(MSG* pMsg)
{
	return IsDialogMessage(pMsg);
}

BOOL CFindDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
	DlgResize_Init();

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != nullptr);
	pLoop->AddMessageFilter(this);

	return TRUE;
}

void CFindDlg::OnDestroy()
{
	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != nullptr);
	pLoop->RemoveMessageFilter(this);
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

} // namespace debugviewpp 
} // namespace fusion
