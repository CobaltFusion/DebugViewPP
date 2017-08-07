#pragma once

// try not to use CString
//#define _WTL_USE_CSTRING 

#include <atlbase.h>       // base ATL classes
#include <atlapp.h>        // base WTL classes
extern CAppModule _Module; // WTL version of CComModule
#include <atlwin.h>        // ATL GUI classes
#include <atlframe.h>      // WTL frame window classes
// #include <atlmisc.h>       // WTL utility classes like CString // incompatible with _WTL_USE_CSTRING + CustomTabCtrl.h 
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

	LRESULT OnSize(UINT nType, CSize Extent);
	BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	void DisablePaneHeader(CPaneContainer& panecontainer);
	void AddTab(const std::wstring name);
	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

private:
	std::vector<std::shared_ptr<CLogViewTabItem2>> m_tabitems;
};

} // namespace fusion
