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
#include "PipeReader.h"

namespace fusion {
namespace debugviewpp {

struct SelectionInfo;

class CLogViewTabItem : public CTabViewTabItem
{
public:
	void SetView(const std::shared_ptr<CLogView>& pView);
	CLogView& GetView();

private:
	 std::shared_ptr<CLogView> m_pView;
};

class CMainFrame :
	public CTabbedFrameImpl<CMainFrame, CDotNetTabCtrl<CLogViewTabItem>>,
	public CUpdateUI<CMainFrame>,
	public CMessageFilter,
	public CIdleHandler
{
public:
	typedef CTabbedFrameImpl<CMainFrame, CDotNetTabCtrl<CLogViewTabItem>> TabbedFrame;

	CMainFrame();
	~CMainFrame();

	DECLARE_FRAME_WND_CLASS(nullptr, IDR_MAINFRAME)

	BEGIN_UPDATE_UI_MAP(CMainFrame)
	    UPDATE_ELEMENT(ID_LOG_PAUSE, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
	    UPDATE_ELEMENT(ID_LOG_GLOBAL, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_SCROLL, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
	    UPDATE_ELEMENT(ID_VIEW_TIME, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_VIEW_BOOKMARK, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_VIEW_COLUMN_LINE, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_COLUMN_TIME, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_COLUMN_PID, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_COLUMN_PROCESS, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_OPTIONS_LINKVIEWS, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_OPTIONS_AUTONEWLINE, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_OPTIONS_ALWAYSONTOP, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_OPTIONS_HIDE, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_DEFAULT_PANE, UPDUI_STATUSBAR)
		UPDATE_ELEMENT(ID_SELECTION_PANE, UPDUI_STATUSBAR)
		UPDATE_ELEMENT(ID_VIEW_PANE, UPDUI_STATUSBAR)
		UPDATE_ELEMENT(ID_LOGFILE_PANE, UPDUI_STATUSBAR)
		UPDATE_ELEMENT(ID_MEMORY_PANE, UPDUI_STATUSBAR)
	END_UPDATE_UI_MAP()

	void Load(const std::wstring& fileName);
	void Load(HANDLE hFile);
	void Load(std::istream& is);
	void CapturePipe(HANDLE hPipe);
	void FindNext(const std::wstring& text);
	void FindPrevious(const std::wstring& text);

private:
	enum
	{
		WM_FIRST = WM_APP,
		WM_SYSTEMTRAYICON,
	};

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
	void UpdateUI();
	void UpdateStatusBar();
	bool LoadSettings();
	void SaveSettings();

	bool IsPaused() const;
	void Pause();
	void Resume();
	bool GetAutoNewLine() const;
	void SetAutoNewLine(bool value);
	bool GetAlwaysOnTop() const;
	void SetAlwaysOnTop(bool value);

	void AddFilterView();
	void AddFilterView(const std::wstring& name, const LogFilter& filter = LogFilter());
	bool IsDbgViewClearMessage(const std::string& text) const;
	void AddMessage(const Message& message);

	void ClearLog();
	void SaveLogFile(const std::wstring& fileName);
	void SaveViewFile(const std::wstring& fileName);

	LRESULT OnSysCommand(UINT nCommand, CPoint);
	LRESULT OnSystemTrayIcon(UINT, WPARAM wParam, LPARAM lParam);
	LRESULT OnScRestore(UINT, INT, HWND);
	LRESULT OnScClose(UINT, INT, HWND);
	void OnDropFiles(HDROP hDropInfo);
	LRESULT OnBeginTabDrag(NMHDR* pnmh);
	LRESULT OnChangingTab(NMHDR* pnmh);
	LRESULT OnChangeTab(NMHDR* pnmh);
	LRESULT OnCloseTab(NMHDR* pnmh);
	LRESULT OnDeleteTab(NMHDR* pnmh);
	void OnFileNewTab(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnFileOpen(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnFileSaveLog(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnFileSaveView(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnLogClear(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnLinkViews(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnAutoNewline(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnHide(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnAlwaysOnTop(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnLogPause(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnLogGlobal(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnViewFind(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnViewFont(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnViewFilter(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);
	void OnAppAbout(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);

	int GetViewCount() const;
	CLogView& GetView(int i);
	CLogView& GetView();
	void SetLogFont();
	void SetTitle(const std::wstring& title = L"");

	CCommandBarCtrl m_cmdBar;
	CEdit m_findBox;
	CMultiPaneStatusBarCtrl m_statusBar;
	UINT_PTR m_timer;
	double m_timeOffset;
	LogFile m_logFile;
	int m_filterNr;
	CFindDlg m_findDlg;
	HFont m_hFont;
	bool m_linkViews;
	bool m_autoNewLine;
	bool m_hide;
	bool m_tryGlobal;
	std::unique_ptr<DBWinReader> m_pLocalReader;
	std::unique_ptr<DBWinReader> m_pGlobalReader;
	std::unique_ptr<PipeReader> m_pPipeReader;
	std::wstring m_logFileName;
	std::wstring m_txtFileName;
	size_t m_initialPrivateBytes;
	NOTIFYICONDATA m_notifyIconData;
	LOGFONT m_logfont;
	std::wstring m_applicationName;
};

} // namespace debugviewpp 
} // namespace fusion
