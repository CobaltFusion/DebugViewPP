// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "CobaltFusion/Str.h"
#include "CobaltFusion/AtlWinExt.h"
#include "CobaltFusion/fusionassert.h"
#include "Win32/Utilities.h"
#include "RegExDlg.h"

namespace fusion {
namespace debugviewpp {

BEGIN_MSG_MAP2(CRegExDlg)
    MSG_WM_INITDIALOG(OnInitDialog)
    COMMAND_ID_HANDLER_EX(IDCANCEL, OnClose)
    COMMAND_ID_HANDLER_EX(IDOK, OnClose)
    REFLECT_NOTIFICATIONS()
END_MSG_MAP()

void CRegExDlg::OnException() const
{
    FUSION_REPORT_EXCEPTION("Unknown Exception");
}

void CRegExDlg::OnException(const std::exception& ex) const
{
    FUSION_REPORT_EXCEPTION(ex.what());
}

BOOL CRegExDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    CenterWindow(GetParent());
    return TRUE;
}

void CRegExDlg::OnClose(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    ShowWindow(SW_HIDE);
}

} // namespace debugviewpp
} // namespace fusion
