// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include "CobaltFusion/AtlWinExt.h"

namespace fusion {
namespace debugviewpp {

class CFileOptionDlg : public CFileDialogImpl<CFileOptionDlg>
{
public:
    CFileOptionDlg(
        BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
        LPCTSTR lpszOptionLabel,
        LPCTSTR lpszDefExt = nullptr,
        LPCTSTR lpszFileName = nullptr,
        DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        LPCTSTR lpszFilter = nullptr,
        HWND hWndParent = nullptr);

    bool Option() const;

    BEGIN_MSG_MAP(CFileOpenDlg)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_DESTROY(OnDestroy)
        CHAIN_MSG_MAP(CFileDialogImpl<CFileOptionDlg>)
    END_MSG_MAP()

private:
    BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
    void OnDestroy();

    std::wstring m_label;
    bool m_option;
};

} // namespace debugviewpp
} // namespace fusion
