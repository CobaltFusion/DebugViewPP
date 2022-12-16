// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "Win32/Utilities.h"
#include "CobaltFusion/stringbuilder.h"
#include "resource.h"
#include "version.h"
#include "AboutDlg.h"

#include <array>

namespace fusion {
namespace debugviewpp {

BOOL CAboutDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    CenterWindow(GetParent());
    m_srclink.SubclassWindow(GetDlgItem(IDC_DEBUGVIEW_SRC_URL));
    m_link.SubclassWindow(GetDlgItem(IDC_DEBUGVIEW_URL));
    std::array<int, 4> version = {VERSION};
    SetDlgItemText(IDC_VERSION, WStr(wstringbuilder() << L"DebugView++ V" << version[0] << L"." << version[1] << L"." << version[2] << L"." << version[3]));
    SetDlgItemText(IDC_DATE, _T(__DATE__));
    return TRUE;
}

void CAboutDlg::OnCloseCmd(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
    EndDialog(nID);
}

} // namespace debugviewpp
} // namespace fusion
