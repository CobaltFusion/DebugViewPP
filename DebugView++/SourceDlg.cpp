// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <boost/algorithm/string.hpp>
#include <atlstr.h>
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
	REFLECT_NOTIFICATIONS()
	CHAIN_MSG_MAP(CDialogResize<CSourceDlg>)
END_MSG_MAP_CATCH(ExceptionHandler)

CSourceDlg::CSourceDlg(const std::shared_ptr<LogSource>& pLogSource) : m_pLogSource(pLogSource)
{
}

void CSourceDlg::ExceptionHandler()
{
	MessageBox(WStr(GetExceptionMessage()).c_str(), LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
}

BOOL CSourceDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
	CenterWindow(GetParent());
	m_combo = GetDlgItem(IDC_TYPE);
	m_combo.AddString(WStr(SourceTypeToString(SourceType::UDP_Socket)));
	m_combo.AddString(WStr(SourceTypeToString(SourceType::TCP_Socket)));
	m_combo.AddString(WStr(SourceTypeToString(SourceType::Debugview_Agent)));
	DlgResize_Init();
	return TRUE;
}

void CSourceDlg::OnCancel(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
	EndDialog(nID);
}

void CSourceDlg::OnOk(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
	m_name = fusion::GetDlgItemText(*this, IDC_NAME);
	m_port = GetDlgItemInt(IDC_PORT);
	m_address = fusion::GetDlgItemText(*this, IDC_IPADDRESS);
	m_sourcetype = StringToSourceType(Str(fusion::GetDlgItemText(*this, IDC_TYPE)));
	EndDialog(nID);
}

} // namespace debugviewpp 
} // namespace fusion
