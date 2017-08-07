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
    m_listviewctrl.Create(*this, rect, nullptr, WS_CHILD | WS_CLIPCHILDREN);

    //m_list.SetItemCount(5);
    m_listviewctrl.SetBkColor(RGB(10, 10, 255));

    m_listviewctrl.InsertColumn(0, _T("Scoobies"), LVCFMT_LEFT, 100, 0);
    m_listviewctrl.InsertItem(0, _T("Willow"));
    m_listviewctrl.InsertItem(1, _T("Buffy"));
    m_listviewctrl.InsertItem(2, _T("Giles"));
    m_listviewctrl.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
    m_listviewctrl.ShowWindow(true);

    return 0;
}


