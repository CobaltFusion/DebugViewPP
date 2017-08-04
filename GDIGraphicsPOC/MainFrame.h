#pragma once

#include <Shellapi.h>

#include <atlapp.h>
#include <atlctrls.h>
#include <atlctrlx.h>

#include <atlbase.h> // Base ATL classes
#include <atlwin.h> // ATL windowing classes
#include <atlsplit.h>
#include <atlframe.h>
#include "CobaltFusion/dbgstream.h"
#include "CobaltFusion/Str.h"
#include "CobaltFusion/stringbuilder.h"
#include "Logview.h"
#include <iomanip>

#pragma warning(push, 3)
#pragma warning(disable : 4838)


#include <atlgdix.h> // CustomTabCtrl.h prerequisites
#include "atlcoll.h"
#include "CustomTabCtrl.h"
#include "DotNetTabCtrl.h"
#include "TabbedFrame.h"

#include "AtlWinExt.h"
#pragma warning(pop)

#include "TabItem.h"

namespace fusion {

class CMainFrame : public CTabbedFrameImpl<CMainFrame, CDotNetTabCtrl<CLogViewTabItem2>>
{
public:
    DECLARE_WND_CLASS(_T("CMainFrame Class"))

    BEGIN_MSG_MAP(CMainFrame)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MSG_WM_MOUSEWHEEL(OnMouseWheel)
        MSG_WM_SIZE(OnSize)
        DEFAULT_REFLECTION_HANDLER()
    END_MSG_MAP()

    LRESULT OnSize(UINT nType, CSize Extent)
    {
        cdbg << "OnSize:: " << Extent.cx << ", " << Extent.cy << "\n";
        UpdateLayout();
        return 1;
    }

    class ScaleData
    {
    public:
        ScaleData()
        {
        }

        int startBase = 0;
        int startPower = 1;

        int endBase = 1;
        int endPower = -3;

        int GetStart()
        {
            return startBase;
        }

        int GetEnd()
        {
            return endBase;
        }

        std::string GetUnit()
        {
            return "ms";
        }
    };

    BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
    {
        //if (zDelta > 0)
        //{
        //	m_timelineView.Zoom(2.0);
        //}
        //else
        //{
        //	m_timelineView.Zoom(0.5);
        //}
        return TRUE;
    }

    void DisablePaneHeader(CPaneContainer& panecontainer)
    {
        panecontainer.SetPaneContainerExtendedStyle(PANECNT_NOCLOSEBUTTON, 0);
        panecontainer.m_cxyHeader = 0;
    }

    void AddTab(const std::wstring name)
    {
        auto lvi = std::make_shared<CLogViewTabItem2>();
        m_tabitems.push_back(lvi);
        lvi->Create(*this);
        lvi->SetText(name.c_str());

        int newIndex = GetTabCtrl().GetItemCount();
        GetTabCtrl().InsertItem(newIndex, lvi.get());
        GetTabCtrl().SetCurSel(newIndex);
    }

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        auto rect = RECT();
        GetClientRect(&rect);

        // block 1
        //auto lvi = std::make_shared<CLogViewTabItem2>();
        //m_tabitems.push_back(lvi);
        //auto& timeline = lvi->GetTimeLineView();
        //      lvi->InitTimeLine();
        //      timeline.Create(*this, CWindow::rcDefault, gdi::CTimelineView::GetWndClassName(),
        //          WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | SS_OWNERDRAW);

        //      timeline.SetView(0.0, 1000.0);

        // block 2
        CreateTabWindow(*this, rect, CTCS_CLOSEBUTTON | CTCS_DRAGREARRANGE);
        AddTab(L"Tab1");
        AddTab(L"Tab2");

        ShowTabControl();
        return 0;
    }

    LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        DestroyWindow();
        return 0;
    }

    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        PostQuitMessage(0);
        return 0;
    }
    std::vector<std::shared_ptr<CLogViewTabItem2>> m_tabitems;
    CLogView m_logview;
};

} // namespace fusion
