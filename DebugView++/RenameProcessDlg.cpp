// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "CobaltFusion/AtlWinExt.h"
#include "CobaltFusion/fusionassert.h"
#include "RenameProcessDlg.h"

#include <utility>
#include "CobaltFusion/Str.h"
#include "CobaltFusion/fusionassert.h"

namespace fusion {
namespace debugviewpp {

BEGIN_MSG_MAP2(CRenameProcessDlg)
    MSG_WM_INITDIALOG(OnInitDialog)
    COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
    COMMAND_ID_HANDLER_EX(IDOK, OnOk)
    COMMAND_HANDLER_EX(IDC_TYPE, CBN_SELCHANGE, OnTypeSelChange)
END_MSG_MAP()

CRenameProcessDlg::CRenameProcessDlg(std::wstring name) :
    m_name(std::move(name))
{
}

std::wstring CRenameProcessDlg::GetName() const
{
    return m_name;
}

void CRenameProcessDlg::OnException()
{
    FUSION_REPORT_EXCEPTION("Unknown Exception");
}

void CRenameProcessDlg::OnException(const std::exception& ex)
{
    FUSION_REPORT_EXCEPTION(ex.what());
}

BOOL CRenameProcessDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    CenterWindow(GetParent());
    SetDlgItemText(IDC_NAME, m_name.c_str());
    return TRUE;
}

void CRenameProcessDlg::OnCancel(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
    EndDialog(nID);
}

void CRenameProcessDlg::OnOk(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
    m_name = Win32::GetDlgItemText(*this, IDC_NAME);
    EndDialog(nID);
}

void CRenameProcessDlg::OnTypeSelChange(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
}

} // namespace debugviewpp
} // namespace fusion
