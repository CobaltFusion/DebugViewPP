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
#include "LogView.h"
#include "DBWinReader.h"

namespace gj {

struct SelectionInfo;

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
		UPDATE_ELEMENT(ID_LOG_SCROLL, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_LOG_AUTONEWLINE, UPDUI_MENUPOPUP)
	    UPDATE_ELEMENT(ID_LOG_TIME, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
	    UPDATE_ELEMENT(ID_LOG_PAUSE, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_DEFAULT_PANE, UPDUI_STATUSBAR)
	END_UPDATE_UI_MAP()

	void SetLineRange(const SelectionInfo& selection);
	void FindNext(const std::wstring& text);
	void FindPrevious(const std::wstring& text);
	void UpdateUI();

private:
	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID);
	void ExceptionHandler();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle();

	LRESULT OnCreate(const CREATESTRUCT* pCreate);
	void OnClose();
	void OnTimer(UINT_PTR nIDEvent);
	void ProcessLines(const LinesList& lines);

	void UpdateStatusBar();
	bool LoadSettings();
	void SaveSettings();

	void AddFilterView();
	void AddFilterView(const std::wstring& name, std::vector<LogFilter> filters = std::vector<LogFilter>());
	void AddMessage(const Message& msg);
	void AddPreppedMessage(const Message& msg);

	std::wstring GetLogFileName() const;
	void SaveLogFile(const std::wstring& fileName);

	LRESULT OnClickTab(NMHDR* pnmh);
	LRESULT OnChangeTab(NMHDR* pnmh);
	LRESULT OnCloseTab(NMHDR* pnmh);
	void OnFileNewTab(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnFileSave(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnFileSaveAs(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnLogSelectAll(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnLogClear(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnLogScroll(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnLogTime(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnAutoNewline(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnLogFilter(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnLogCopy(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnLogPause(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnLogFind(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnViewFont(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnAppAbout(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);

	CLogView& GetView();
	void SetLogFont();

//	GuiThread m_guiThread;
	UINT_PTR m_timer;
	double m_timeOffset;
	LogFile m_logFile;
	int m_filterNr;
	std::vector<std::unique_ptr<CLogView>> m_views;
	CFontDialog m_fontDlg;
	CFindDlg m_findDlg;
	bool m_autoNewLine;
	bool m_paused;
	std::unique_ptr<DBWinReader> m_localReader;
	std::unique_ptr<DBWinReader> m_globalReader;
	boost::signals2::connection m_localConnection;
	boost::signals2::connection m_globalConnection;
	std::wstring m_logFileName;
};

} // namespace gj
