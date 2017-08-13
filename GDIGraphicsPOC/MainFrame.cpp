#include "stdafx.h"

#include "MainFrame.h"
#include "CMainFrame2.h"

CAppModule _Module;

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, LPSTR szCmdLine, int nCmdShow)
{
    // CMainFrame is a CTabbedFrameImpl<> and does not render as intended
    // CMainFrame2 is a CFrameWindowImpl<> and _does_ render as intended
    
    fusion::CMainFrame wndMain;

    // Create & show our main window
    if (nullptr == wndMain.Create(nullptr, CWindow::rcDefault, _T("WTL Frame"), WS_OVERLAPPEDWINDOW))
    {
        // Bad news, window creation failed
        return 1;
    }

    wndMain.ShowWindow(nCmdShow);
    wndMain.UpdateWindow();

    // Run the message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}

namespace fusion
{

    LRESULT CMainFrame::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        DestroyWindow();
        return 0;
    }

    LRESULT CMainFrame::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        PostQuitMessage(0);
        return 0;
    }

    LRESULT CMainFrame::OnSize(UINT nType, CSize Extent)
    {
        cdbg << "OnSize:: " << Extent.cx << ", " << Extent.cy << "\n";
        UpdateLayout();
        return 1;
    }

    void CMainFrame::AddTab(const std::wstring name)
    {
        auto lvi = std::make_shared<CLogViewTabItem2>();
        m_tabitems.push_back(lvi);
        lvi->SetText(name.c_str());
        lvi->Create(*this);

        int newIndex = GetTabCtrl().GetItemCount();
        GetTabCtrl().InsertItem(newIndex, lvi.get());
        GetTabCtrl().SetCurSel(newIndex);
    }

    LRESULT CMainFrame::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        CreateTabWindow(*this, CWindow::rcDefault, CTCS_CLOSEBUTTON | CTCS_DRAGREARRANGE);
        AddTab(L"Tab1");
        AddTab(L"Tab2");
        ShowTabControl();
        return 0;
    }

}