// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <boost/algorithm/string.hpp>
#include <atlstr.h>
#include "CobaltFusion/Str.h"
#include "CobaltFusion/AtlWinExt.h"
#include "Win32/utilities.h"
#include "DebugView++Lib/LogFilter.h"
#include "DebugView++Lib/LogSource.h"
#include "Resource.h"
#include "SourceDlg.h"

namespace fusion {
namespace debugviewpp {

BEGIN_MSG_MAP_TRY(CSourceDlg)
	MSG_WM_INITDIALOG(OnInitDialog)
	COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
	COMMAND_ID_HANDLER_EX(IDOK, OnOk)
	COMMAND_HANDLER_EX(IDC_TYPE, CBN_SELCHANGE, OnTypeSelChange)
END_MSG_MAP_CATCH(ExceptionHandler)

CSourceDlg::CSourceDlg(const std::wstring& name, SourceType::type sourceType, const std::wstring& address, int port) :
	m_name(name), m_sourceType(sourceType), m_address(address), m_port(port)
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

void CSourceDlg::ExceptionHandler()
{
	MessageBox(WStr(GetExceptionMessage()), LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
}

BOOL CSourceDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
	CenterWindow(GetParent());
	CComboBox combo = GetDlgItem(IDC_TYPE);
	combo.AddString(WStr(SourceTypeToString(SourceType::UDP_Socket)));
	combo.AddString(WStr(SourceTypeToString(SourceType::TCP_Socket)));
	combo.AddString(WStr(SourceTypeToString(SourceType::Debugview_Agent)));

	SetDlgItemText(IDC_NAME, WStr(m_name));
	switch (m_sourceType)
	{
	case SourceType::UDP_Socket: combo.SetCurSel(0); break;
	case SourceType::TCP_Socket: combo.SetCurSel(1); break;
	case SourceType::Debugview_Agent: combo.SetCurSel(2); break;
	default: combo.SetCurSel(0); break;
	}
	SetDlgItemInt(IDC_PORT, m_port);
	SetDlgItemText(IDC_IPADDRESS, WStr(m_address));
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
	GetDlgItem(IDC_PORT).EnableWindow(sourceType == SourceType::UDP_Socket || sourceType == SourceType::TCP_Socket);
	GetDlgItem(IDC_IPADDRESS).EnableWindow(sourceType == SourceType::Debugview_Agent);
}

} // namespace debugviewpp 
} // namespace fusion
