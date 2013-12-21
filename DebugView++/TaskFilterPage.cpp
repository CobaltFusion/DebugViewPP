// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "TaskFilterPage.h"
#include "resource.h"

namespace fusion {

BEGIN_MSG_MAP_TRY(CTaskFilterPage)
	MSG_WM_INITDIALOG(OnInitDialog)
	MSG_WM_DESTROY(OnDestroy)
	REFLECT_NOTIFICATIONS()
	CHAIN_MSG_MAP(CDialogResize<CTaskFilterPage>)
END_MSG_MAP_CATCH(ExceptionHandler)

void CTaskFilterPage::ExceptionHandler()
{
	MessageBox(WStr(GetExceptionMessage()), LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
}


BOOL CTaskFilterPage::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
	m_grid.SubclassWindow(GetDlgItem(IDC_GRID));
	m_grid.InsertColumn(0, L"", LVCFMT_LEFT, 32, 0);
	m_grid.InsertColumn(1, L"Filter", LVCFMT_LEFT, 200, 0);
	m_grid.InsertColumn(2, L"Type", LVCFMT_LEFT, 60, 0);
	m_grid.InsertColumn(3, L"Bg", LVCFMT_LEFT, 20, 0);
	m_grid.InsertColumn(4, L"Fg", LVCFMT_LEFT, 20, 0);
	m_grid.InsertColumn(5, L"", LVCFMT_LEFT, 16, 0);
	m_grid.SetExtendedGridStyle(PGS_EX_SINGLECLICKEDIT | PGS_EX_ADDITEMATEND);

	DlgResize_Init(false);

	return TRUE;
}

void CTaskFilterPage::OnDestroy()
{
}

} // namespace fusion
