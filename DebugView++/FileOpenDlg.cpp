// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "resource.h"
#include "version.h"
#include "FileOpenDlg.h"

namespace fusion {
namespace debugviewpp {

CFileOpenDlg::CFileOpenDlg(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
	LPCTSTR lpszDefExt,
	LPCTSTR lpszFileName,
	DWORD dwFlags,
	LPCTSTR lpszFilter,
	HWND hWndParent) 
		: CFileDialogImpl<CFileOpenDlg>(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, hWndParent)
		, m_keep(false)
{
}

bool CFileOpenDlg::Keep() const
{
	return m_keep;
}

BOOL CFileOpenDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
	// SetControlText(chx1, L"Keep file open");
	return TRUE;
}

void CFileOpenDlg::OnDestroy()
{
	m_keep = GetReadOnlyPref() != FALSE;
}

} // namespace debugviewpp 
} // namespace fusion
