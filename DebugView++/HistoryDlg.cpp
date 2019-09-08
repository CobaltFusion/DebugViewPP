// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "CobaltFusion/AtlWinExt.h"
#include "CobaltFusion/Str.h"
#include "CobaltFusion/fusionassert.h"
#include "Win32/Utilities.h"
#include "HistoryDlg.h"

namespace fusion {
namespace debugviewpp {

BEGIN_MSG_MAP2(CHistoryDlg)
    MSG_WM_INITDIALOG(OnInitDialog)
    COMMAND_ID_HANDLER_EX(IDC_UNLIMITED, OnUnlimited)
    COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
    COMMAND_ID_HANDLER_EX(IDOK, OnOk)
    REFLECT_NOTIFICATIONS()
END_MSG_MAP()

CHistoryDlg::CHistoryDlg(size_t historySize, bool unlimited) :
    m_historySize(static_cast<int>(historySize)),
    m_unlimited(unlimited)
{
}

void CHistoryDlg::OnException() const
{
    FUSION_REPORT_EXCEPTION("Unknown Exception");
}

void CHistoryDlg::OnException(const std::exception& ex) const
{
    FUSION_REPORT_EXCEPTION(ex.what());
}

BOOL CHistoryDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    SetDlgItemInt(IDC_HISTORY, m_historySize);

    CButton unlimited(GetDlgItem(IDC_UNLIMITED));
    unlimited.SetCheck(static_cast<int>(m_unlimited));
    UpdateUi();

    CenterWindow(GetParent());

    return TRUE;
}

void CHistoryDlg::OnUnlimited(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CButton unlimited(GetDlgItem(IDC_UNLIMITED));
    m_unlimited = unlimited.GetCheck() == BST_CHECKED;
    UpdateUi();
}

void CHistoryDlg::UpdateUi() const
{
    GetDlgItem(IDC_HISTORY).EnableWindow(static_cast<BOOL>(!m_unlimited));
}

void CHistoryDlg::OnCancel(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
    EndDialog(nID);
}

void CHistoryDlg::OnOk(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
    m_historySize = GetDlgItemInt(IDC_HISTORY);
    //    m_unlimited = fusion::GetDlgItemText(*this, IDC_ARGUMENTS);

    EndDialog(nID);
}

int CHistoryDlg::GetHistorySize() const
{
    return m_historySize;
}

} // namespace debugviewpp
} // namespace fusion
