// #include "stdafx.h"
#include "windows.h"
#include <string>

#define WIN32_LEAN_AND_MEAN

#include "CMainFrame2.h"

LRESULT fusion::CMainFrame2::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
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

LRESULT fusion::CMainFrame2::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    auto rect = RECT();
    GetClientRect(&rect);
    rect.bottom = 540; // 40x 13 = 520, + 20 for column header

    m_logview.Create(*this, rect);
    AddDummyContent(m_logview);
    return 0;
}


