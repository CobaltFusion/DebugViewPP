// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "resource.h"
#include "MainFrame.h"
#include "FindDlg.h"

namespace fusion {
namespace debugviewpp {

CFindDlg::CFindDlg(CMainFrame& mainFrame) :
    m_mainFrame(mainFrame)
{
}

BOOL CFindDlg::PreTranslateMessage(MSG* pMsg)
{
    return IsDialogMessage(pMsg);
}

BOOL CFindDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    DlgResize_Init();

    // register object for message filtering and idle updates
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != nullptr);
    pLoop->AddMessageFilter(this);

    return TRUE;
}

void CFindDlg::OnDestroy()
{
    // register object for message filtering and idle updates
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    if (pLoop != nullptr)
    { // shutdown bug found by PVS studio
        pLoop->RemoveMessageFilter(this);
    }
}

void CFindDlg::OnNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/) const
{
    m_mainFrame.FindNext(Win32::GetDlgItemText(*this, IDC_TEXT));
}

void CFindDlg::OnPrevious(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/) const
{
    m_mainFrame.FindPrevious(Win32::GetDlgItemText(*this, IDC_TEXT));
}

} // namespace debugviewpp
} // namespace fusion
