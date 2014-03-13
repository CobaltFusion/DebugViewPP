// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <boost/algorithm/string.hpp>
#include <atlstr.h>
#include "Win32Lib/utilities.h"
#include "DebugView++Lib/LogFilter.h"
#include "Resource.h"
#include "SourcesDlg.h"

namespace fusion {
namespace debugviewpp {

BEGIN_MSG_MAP_TRY(CSourcesDlg)
	MSG_WM_INITDIALOG(OnInitDialog)
	COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
	COMMAND_ID_HANDLER_EX(IDOK, OnOk)
	REFLECT_NOTIFICATIONS()
	CHAIN_MSG_MAP(CDialogResize<CSourcesDlg>)
END_MSG_MAP_CATCH(ExceptionHandler)

CSourcesDlg::CSourcesDlg()
{
}

void CSourcesDlg::ExceptionHandler()
{
	MessageBox(WStr(GetExceptionMessage()).c_str(), LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
}

BOOL CSourcesDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
	SetDlgItemText(IDC_NAME, L"test");

	m_tabCtrl.Attach(GetDlgItem(IDC_TAB));
	m_tabCtrl.AddItem(L"Sources");

	CRect tabRect;
	m_tabCtrl.GetWindowRect(&tabRect);
	m_tabCtrl.AdjustRect(false, &tabRect);
	m_tabCtrl.ScreenToClient(&tabRect);

	CRect dlgRect;
	GetClientRect(&dlgRect);
	m_border.cx = dlgRect.Width() - tabRect.Width();
	m_border.cy = dlgRect.Height() - tabRect.Height();

	m_sourcesPage.Create(m_tabCtrl, tabRect);
	m_sourcesPage.MoveWindow(&tabRect);
	m_sourcesPage.ShowWindow(SW_SHOW);

	CenterWindow(GetParent());
	DlgResize_Init();
	return TRUE;
}

void CSourcesDlg::OnCancel(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
	EndDialog(nID);
}

void CSourcesDlg::OnOk(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
	EndDialog(nID);
}

} // namespace debugviewpp 
} // namespace fusion
