// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Dlgs.h"
#include "FileOpenDlg.h"

namespace fusion {
namespace debugviewpp {

CFileOptionDlg::CFileOptionDlg(
	BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
	LPCTSTR lpszOptionLabel,
	LPCTSTR lpszDefExt,
	LPCTSTR lpszFileName,
	DWORD dwFlags,
	LPCTSTR lpszFilter,
	HWND hWndParent) :
	CFileDialogImpl<CFileOptionDlg>(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, hWndParent),
	m_label(lpszOptionLabel),
	m_option(false)
{
}

bool CFileOptionDlg::Option() const
{
	return m_option;
}

BOOL CFileOptionDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
	// customize file dialog, using "Explorer-Style Control Identifiers", see:
	// https://msdn.microsoft.com/en-us/library/windows/desktop/ms646960(v=vs.85).aspx

	SetControlText(chx1, m_label.c_str());
	return TRUE;
}

void CFileOptionDlg::OnDestroy()
{
	m_option = GetReadOnlyPref() != FALSE;
}

} // namespace debugviewpp 
} // namespace fusion
