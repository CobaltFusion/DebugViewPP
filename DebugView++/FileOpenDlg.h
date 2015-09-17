// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "Resource.h"

namespace fusion {
namespace debugviewpp {

class CFileOpenDlg : public CFileDialogImpl<CFileOpenDlg>
{
public:
	CFileOpenDlg(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
	LPCTSTR lpszDefExt = nullptr,
	LPCTSTR lpszFileName = nullptr,
	DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
	LPCTSTR lpszFilter = nullptr,
	HWND hWndParent = nullptr);

	bool Keep() const;

	BEGIN_MSG_MAP(CFileOpenDlg)
	MSG_WM_INITDIALOG(OnInitDialog)
	MSG_WM_DESTROY(OnDestroy)
	CHAIN_MSG_MAP(CFileDialogImpl<CFileOpenDlg>)
	END_MSG_MAP()

private:
	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnDestroy();

	bool m_keep;
};

} // namespace debugviewpp 
} // namespace fusion
