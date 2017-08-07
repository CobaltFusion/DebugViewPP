#pragma once

#include <atlbase.h>       // base ATL classes
#include <atlapp.h>        // base WTL classes
extern CAppModule _Module; // WTL version of CComModule
#include <atlwin.h>        // ATL GUI classes
#include <atlframe.h>      // WTL frame window classes
#include <atlcrack.h>      // WTL enhanced msg map macros

// CustomTabCtrl.h prerequisites
#include "atlctrls.h"
#include "atlgdix.h"
#include "atlcoll.h"

#pragma warning(push, 3)
#pragma warning(disable : 4838)

#include "CustomTabCtrl.h"
#include "TabbedFrame.h"

#pragma warning(pop)

#include "AtlWinExt.h"

#include "Logview.h"
#include "TabItem.h"

namespace fusion {

typedef CCheckListViewCtrlImplTraits<
    0, 0, LVS_EX_CHECKBOXES | LVS_EX_GRIDLINES | LVS_EX_UNDERLINEHOT |
    LVS_EX_ONECLICKACTIVATE> CMyCheckListTraits;

class CMyCheckListCtrl :
    public CCheckListViewCtrlImpl<CMyCheckListCtrl, CListViewCtrl, CMyCheckListTraits>
{
private:
    typedef CCheckListViewCtrlImpl<CMyCheckListCtrl, CListViewCtrl, CMyCheckListTraits> baseClass;
public:
    DECLARE_WND_SUPERCLASS(_T("WTL_CheckListView"), GetWndClassName())

    BEGIN_MSG_MAP(CMyCheckListCtrl)
        CHAIN_MSG_MAP(baseClass)
    END_MSG_MAP()
};

class CMainFrame2 : public WTL::CFrameWindowImpl<CMainFrame2, ATL::CWindow, ATL::CFrameWinTraits>
{
public:
    DECLARE_FRAME_WND_CLASS(_T("First WTL window"), IDR_MAINFRAME);

    BEGIN_MSG_MAP(CMainFrame2)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MSG_WM_MOUSEWHEEL(OnMouseWheel)
        MSG_WM_SIZE(OnSize)
        CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame2>)
        DEFAULT_REFLECTION_HANDLER()
    END_MSG_MAP()

    LRESULT OnSize(UINT nType, CSize Extent);
    BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    void DisablePaneHeader(CPaneContainer& panecontainer);
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
    CLogView m_logview;
};

class MainWnd : public CWindowImpl<MainWnd>
{
public:
    DECLARE_WND_CLASS(_T("Specific_Class_Name"))

    BEGIN_MSG_MAP(MainWnd)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    END_MSG_MAP()

    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
    {
        PostQuitMessage(0);
        bHandled = FALSE;
        return 0;
    }

};


} // namespace fusion
