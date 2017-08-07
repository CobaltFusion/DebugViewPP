#include "stdafx.h"
#include "windows.h"
#include <string>

#define WIN32_LEAN_AND_MEAN

#include "CMainFrame2.h"

LRESULT fusion::CMainFrame2::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DestroyWindow();
    return 0;
}

LRESULT fusion::CMainFrame2::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PostQuitMessage(0);
    return 0;
}

LRESULT fusion::CMainFrame2::OnSize(UINT nType, CSize Extent)
{
    cdbg << "OnSize:: " << Extent.cx << ", " << Extent.cy << "\n";
    UpdateLayout();
    return 1;
}

BOOL fusion::CMainFrame2::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    return TRUE;
}

void fusion::CMainFrame2::DisablePaneHeader(CPaneContainer& panecontainer)
{
    panecontainer.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON, 0);
    panecontainer.m_cxyHeader = 0;
}

LRESULT fusion::CMainFrame2::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    auto rect = RECT();
    GetClientRect(&rect);
    rect.bottom = 600;
    m_logview.Create(*this, rect, CListViewCtrl::GetWndClassName(), WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | SS_OWNERDRAW, WS_EX_CLIENTEDGE);
    AddDummyContent(m_logview);
    m_logview.SetBkColor(RGB(10, 10, 255));
    return 0;
}


