// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "CobaltFusion/AtlWinExt.h"
#include "CobaltFusion/fusionassert.h"
#include "DebugView++Lib/LogSource.h"
#include "SourceDlg.h"

#include <utility>
#include "CobaltFusion/Str.h"

namespace fusion {
namespace debugviewpp {

BEGIN_MSG_MAP2(CSourceDlg)
    MSG_WM_INITDIALOG(OnInitDialog)
    COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
    COMMAND_ID_HANDLER_EX(IDOK, OnOk)
    COMMAND_HANDLER_EX(IDC_TYPE, CBN_SELCHANGE, OnTypeSelChange)
END_MSG_MAP()

CSourceDlg::CSourceDlg(std::wstring name, SourceType::type sourceType, std::wstring address, int port) :
    m_name(std::move(name)),
    m_sourceType(sourceType),
    m_address(std::move(address)),
    m_port(port)
{
}

std::wstring CSourceDlg::GetName() const
{
    return m_name;
}

SourceType::type CSourceDlg::GetSourceType() const
{
    return m_sourceType;
}

std::wstring CSourceDlg::GetAddress() const
{
    return m_address;
}

int CSourceDlg::GetPort() const
{
    return m_port;
}

void CSourceDlg::OnException()
{
    FUSION_REPORT_EXCEPTION("Unknown Exception");
}

void CSourceDlg::OnException(const std::exception& ex)
{
    FUSION_REPORT_EXCEPTION(ex.what());
}

BOOL CSourceDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    CenterWindow(GetParent());
    CComboBox combo(GetDlgItem(IDC_TYPE));
    combo.AddString(WStr(SourceTypeToString(SourceType::Udp)));
    combo.AddString(WStr(SourceTypeToString(SourceType::Tcp)));
    combo.AddString(WStr(SourceTypeToString(SourceType::DebugViewAgent)));

    SetDlgItemText(IDC_NAME, m_name.c_str());
    switch (m_sourceType)
    {
    case SourceType::Udp: combo.SetCurSel(0); break;
    case SourceType::Tcp: combo.SetCurSel(1); break;
    case SourceType::DebugViewAgent: combo.SetCurSel(2); break;
    default: combo.SetCurSel(0); break;
    }
    SetDlgItemInt(IDC_PORT, m_port);
    SetDlgItemText(IDC_IPADDRESS, m_address.c_str());
    UpdateUI();

    return TRUE;
}

void CSourceDlg::OnCancel(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
    EndDialog(nID);
}

void CSourceDlg::OnOk(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
    m_name = Win32::GetDlgItemText(*this, IDC_NAME);
    m_port = GetDlgItemInt(IDC_PORT);
    m_address = Win32::GetDlgItemText(*this, IDC_IPADDRESS);
    m_sourceType = StringToSourceType(Str(Win32::GetDlgItemText(*this, IDC_TYPE)));
    EndDialog(nID);
}

void CSourceDlg::OnTypeSelChange(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    UpdateUI();
}

void CSourceDlg::UpdateUI()
{
    auto sourceType = StringToSourceType(Str(Win32::GetDlgItemText(*this, IDC_TYPE)));
    GetDlgItem(IDC_PORT).EnableWindow(static_cast<BOOL>(sourceType == SourceType::Udp || sourceType == SourceType::Tcp));
    GetDlgItem(IDC_IPADDRESS).EnableWindow(static_cast<BOOL>(sourceType == SourceType::DebugViewAgent || sourceType == SourceType::Udp || sourceType == SourceType::Tcp));
}

} // namespace debugviewpp
} // namespace fusion
