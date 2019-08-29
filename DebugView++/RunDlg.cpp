// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "CobaltFusion/Str.h"
#include "CobaltFusion/AtlWinExt.h"
#include "CobaltFusion/fusionassert.h"
#include "Win32/Utilities.h"
#include "RunDlg.h"

namespace fusion {
namespace debugviewpp {

BEGIN_MSG_MAP2(CRunDlg)
    MSG_WM_INITDIALOG(OnInitDialog)
    COMMAND_ID_HANDLER_EX(IDC_BROWSE, OnBrowse)
    COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
    COMMAND_ID_HANDLER_EX(IDOK, OnOk)
    REFLECT_NOTIFICATIONS()
    CHAIN_MSG_MAP(CDialogResize<CRunDlg>)
END_MSG_MAP()

void CRunDlg::OnException()
{
    FUSION_REPORT_EXCEPTION("Unknown Exception");
}

void CRunDlg::OnException(const std::exception& ex)
{
    FUSION_REPORT_EXCEPTION(ex.what());
}

BOOL CRunDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    SetDlgItemText(IDC_RUN, m_pathName.c_str());
    SetDlgItemText(IDC_ARGUMENTS, m_arguments.c_str());

    SHAutoComplete(GetDlgItem(IDC_RUN), SHACF_FILESYSTEM);

    CenterWindow(GetParent());
    DlgResize_Init();

    return TRUE;
}

void CRunDlg::OnBrowse(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CFileDialog dlg(true, L".exe", L"", OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
        L"EXE Files (*.exe)\0*.exe\0"
        L"All Files\0*.*\0"
        L"\0",
        nullptr);
    dlg.m_ofn.nFilterIndex = 0;
    dlg.m_ofn.lpstrTitle = L"Select Executable File";

    if (dlg.DoModal() != IDOK)
        return;

    SetDlgItemText(IDC_RUN, dlg.m_szFileName);
}

void CRunDlg::OnCancel(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
    EndDialog(nID);
}

void CRunDlg::OnOk(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
    m_pathName = Win32::GetDlgItemText(*this, IDC_RUN);
    m_arguments = Win32::GetDlgItemText(*this, IDC_ARGUMENTS);

    EndDialog(nID);
}

std::wstring CRunDlg::GetPathName() const
{
    return m_pathName;
}

void CRunDlg::SetPathName(const std::wstring& pathNamde)
{
    m_pathName = pathNamde;
}

std::wstring CRunDlg::GetArguments() const
{
    return m_arguments;
}

} // namespace debugviewpp
} // namespace fusion
