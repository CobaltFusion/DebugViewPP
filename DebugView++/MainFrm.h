//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memory>

#include "atlgdix.h"
#include "atlcoll.h"
namespace WTL { using ATL::CString; };
#include "CustomTabCtrl.h"
#include "DotNetTabCtrl.h"
#include "TabbedFrame.h"

#include "Utilities.h"
//#include "GuiThread.h"
#include "FindDlg.h"
#include "LogFile.h"
#include "DBWinReader.h"

namespace gj {

struct SelectionInfo;
class CLogView;

class CMainFrame :
	public CTabbedFrameImpl<CMainFrame>,
	public CUpdateUI<CMainFrame>,
	public CMessageFilter,
	public CIdleHandler
{
public:
	CMainFrame();
	~CMainFrame();

	DECLARE_FRAME_WND_CLASS(nullptr, IDR_MAINFRAME)

	BEGIN_UPDATE_UI_MAP(CMainFrame)
	    UPDATE_ELEMENT(ID_LOG_TIME, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
	    UPDATE_ELEMENT(ID_LOG_PAUSE, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_DEFAULT_PANE, UPDUI_STATUSBAR)
	END_UPDATE_UI_MAP()

	void SetLineRange(const SelectionInfo& selection);
	void FindNext(const std::wstring& text);
	void FindPrevious(const std::wstring& text);

private:
	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID);
	void ExceptionHandler();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle();

	LRESULT OnCreate(const CREATESTRUCT* pCreate);
	void OnClose();

	LRESULT OnTimer(UINT, WPARAM, LPARAM, BOOL&);

	void UpdateUI();
	bool LoadSettings();
	void SaveSettings();

	void AddFilterView();

	void OnDebugMessage(DWORD processId, const char* text);
	void AddMessage(const Message& msg);

	LRESULT OnClickTab(NMHDR* pnmh);
	LRESULT OnChangeTab(NMHDR* pnmh);
	LRESULT OnCloseTab(NMHDR* pnmh);
	void OnFileSave(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnLogClear(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnLogTime(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnLogFilter(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnLogCopy(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnLogPause(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnLogFind(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnViewFont(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnAppAbout(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);

	CLogView& GetView();
	void SetLogFont();

	//GuiThread m_guiThread;
	UINT_PTR m_timer;
	LogFile m_logFile;
	int m_filterNr;
	std::vector<std::unique_ptr<CLogView>> m_views;
	CFontDialog m_fontDlg;
	CFindDlg m_findDlg;
	bool m_paused;
	DBWinReader m_localReader;
//	DBWinReader m_globalReader;
	boost::signals2::connection m_localConnection;
	boost::signals2::connection m_globalConnection;
};

} // namespace gj
