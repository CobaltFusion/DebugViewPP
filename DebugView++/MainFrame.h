// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

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

namespace fusion {

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
		UPDATE_ELEMENT(ID_LOG_AUTONEWLINE, UPDUI_MENUPOPUP)
	    UPDATE_ELEMENT(ID_LOG_PAUSE, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
	    UPDATE_ELEMENT(ID_LOG_GLOBAL, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_SCROLL, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
	    UPDATE_ELEMENT(ID_VIEW_TIME, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_DEFAULT_PANE, UPDUI_STATUSBAR)
		UPDATE_ELEMENT(ID_SELECTION_PANE, UPDUI_STATUSBAR)
		UPDATE_ELEMENT(ID_VIEW_PANE, UPDUI_STATUSBAR)
		UPDATE_ELEMENT(ID_LOGFILE_PANE, UPDUI_STATUSBAR)
		UPDATE_ELEMENT(ID_MEMORY_PANE, UPDUI_STATUSBAR)
	END_UPDATE_UI_MAP()

	void FindNext(const std::wstring& text);
	void FindPrevious(const std::wstring& text);
	void UpdateUI();
	void SaitUpdate(const std::wstring&);

private:
	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID);
	void ExceptionHandler();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle();

	LRESULT OnCreate(const CREATESTRUCT* pCreate);
	void OnClose();
	void OnTimer(UINT_PTR nIDEvent);
	void ProcessLines(const Lines& lines);

	std::wstring GetSelectionInfoText(const std::wstring& label, const SelectionInfo& selection) const;
	SelectionInfo GetLogFileRange() const;
	void UpdateStatusBar();
	bool LoadSettings();
	void SaveSettings();

	bool GetAutoNewLine() const;
	void SetAutoNewLine(bool value);

	void AddFilterView();
	void AddFilterView(const std::wstring& name, const LogFilter& filter = LogFilter());
	bool IsDbgViewClearMessage(const std::string& text) const;
	void AddMessage(const Message& message);

	std::wstring GetLogFileName() const;
	void SaveLogFile(const std::wstring& fileName);

	LRESULT OnClickTab(NMHDR* pnmh);
	LRESULT OnChangeTab(NMHDR* pnmh);
	LRESULT OnCloseTab(NMHDR* pnmh);
	void OnFileNewTab(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnFileSave(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnFileSaveAs(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnLogClear(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnAutoNewline(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnLogPause(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnLogGlobal(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnViewScroll(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnViewTime(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnViewFind(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnViewFont(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnViewFilter(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnAppAbout(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);

	CLogView& GetView();
	void SetLogFont();

	FILE* m_stdout;
	CMultiPaneStatusBarCtrl m_statusBar;
	UINT_PTR m_timer;
	double m_timeOffset;
	LogFile m_logFile;
	int m_filterNr;
	std::vector<std::unique_ptr<CLogView>> m_views;
	CFontDialog m_fontDlg;
	HFont m_hFont;
	CFindDlg m_findDlg;
	bool m_autoNewLine;
	bool m_localReaderPaused;
	bool m_globalReaderPaused;
	std::unique_ptr<DBWinReader> m_pLocalReader;
	std::unique_ptr<DBWinReader> m_pGlobalReader;
	boost::signals2::connection m_localConnection;
	boost::signals2::connection m_globalConnection;
	std::wstring m_logFileName;
	std::wstring m_saitText;
	ProcessInfo m_processInfo;
	size_t m_initialPrivateBytes;
};

} // namespace fusion
