// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "CobaltFusion/AtlWinExt.h"
#include "resource.h"

#include "atleverything.h"

namespace fusion {
namespace debugviewpp {

class CAboutDlg : public CDialogImpl<CAboutDlg>
{
public:
    enum
    {
        IDD = IDD_ABOUTBOX
    };

    BEGIN_MSG_MAP(CAboutDlg)
        MSG_WM_INITDIALOG(OnInitDialog)
        COMMAND_ID_HANDLER_EX(IDOK, OnCloseCmd)
        COMMAND_ID_HANDLER_EX(IDCANCEL, OnCloseCmd)
    END_MSG_MAP()

    BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
    void OnCloseCmd(UINT uNotifyCode, int nID, CWindow wndCtl);

private:
    CHyperLink m_srclink;
    CHyperLink m_link;
};

} // namespace debugviewpp
} // namespace fusion
