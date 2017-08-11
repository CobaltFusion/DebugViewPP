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

class CMainFrame2 : public WTL::CFrameWindowImpl<CMainFrame2, ATL::CWindow, ATL::CFrameWinTraits>
{
public:
    DECLARE_FRAME_WND_CLASS(_T("First WTL window"), IDR_MAINFRAME);

    BEGIN_MSG_MAP(CMainFrame2)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MSG_WM_SIZE(OnSize)
        CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame2>)
        REFLECT_NOTIFICATIONS();
        DEFAULT_REFLECTION_HANDLER()
    END_MSG_MAP()

    LRESULT OnSize(UINT nType, CSize Extent);
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
    CLogView m_logview;
};

} // namespace fusion
