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

#pragma warning(push, 3)
#pragma warning (disable: 4838)

#include "CustomTabCtrl.h"
#include "DotNetTabCtrl.h"
#include "TabbedFrame.h"
#pragma warning(pop)

#include "CobaltFusion/AtlWinExt.h"
#include "CobaltFusion/ExecutorClient.h"
#include "DebugView++Lib/DBWinBuffer.h"
#include "DebugView++Lib/DBWinReader.h"
#include "DebugView++Lib/LineBuffer.h"
#include "DebugView++Lib/LogSources.h"
#include "DebugView++Lib/FileWriter.h"
#include "FindDlg.h"
#include "RunDlg.h"
#include "LogView.h"

namespace fusion {
namespace debugviewpp {

struct SelectionInfo;
class DbgviewReader;

class CLogViewTabItem : public CTabViewTabItem
{
public:
	void SetView(const std::shared_ptr<CLogView>& pView);
	CLogView& GetView();

private:
	 std::shared_ptr<CLogView> m_pView;
};

class CMultiPaneStatusBarCtrlFlickerFree :
	public CMultiPaneStatusBarCtrlImpl<CMultiPaneStatusBarCtrlFlickerFree>,
	public CDoubleBufferImpl<CMultiPaneStatusBarCtrlFlickerFree>
{
public:
	DECLARE_WND_SUPERCLASS(nullptr, CMultiPaneStatusBarCtrlImpl<CMultiPaneStatusBarCtrlFlickerFree>::GetWndClassName())  

	BEGIN_MSG_MAP(CMultiPaneStatusBarCtrlFlickerFree)
		CHAIN_MSG_MAP(CDoubleBufferImpl<CMultiPaneStatusBarCtrlFlickerFree>)
	END_MSG_MAP()

	void DoPaint(CDCHandle dc)
	{
		RECT rect;
		dc.GetClipBox(&rect);
		dc.FillSolidRect(&rect, Colors::BackGround);
		DefWindowProc(WM_PAINT, reinterpret_cast<WPARAM>(dc.m_hDC), 0);
	}
};

class CMainFrame :
	public CTabbedFrameImpl<CMainFrame, CDotNetTabCtrl<CLogViewTabItem>>,
	public CUpdateUI<CMainFrame>,
	public ExceptionHandler<CMainFrame, std::exception>,
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
		UPDATE_ELEMENT(ID_VIEW_SCROLL_STOP, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_VIEW_TIME, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_VIEW_PROCESSCOLORS, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_BOOKMARK, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
		UPDATE_ELEMENT(ID_VIEW_COLUMN_LINE, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_COLUMN_DATE, UPDUI_MENUPOPUP)
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

	void SetLogging();
	void LoadConfiguration(const std::wstring& fileName);
	void SaveConfiguration(const std::wstring& fileName);
	void Load(const std::wstring& fileName, bool keeptailing);
	void LoadAsync(const std::wstring& fileName);
	void Load(HANDLE hFile);
	void Load(std::istream& is, const std::string& name, FILETIME fileTime);
	void CapturePipe(HANDLE hPipe);
	void FindNext(const std::wstring& text);
	void FindPrevious(const std::wstring& text);
	void OnDropFiles(HDROP hDropInfo);

private:
	enum
	{
		WM_FIRST = WM_APP,
		WM_SYSTEMTRAYICON,
	};

	DECLARE_MSG_MAP()

	BOOL PreTranslateMessage(MSG* pMsg) override;
	BOOL OnIdle() override;

	void OnException();
	void OnException(const std::exception& ex);
	LRESULT OnCreate(const CREATESTRUCT* pCreate);
	void OnClose();
	LRESULT OnQueryEndSession(WPARAM wParam, LPARAM lParam);
	LRESULT OnEndSession(WPARAM wParam, LPARAM lParam);
	bool OnUpdate();
	bool OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	void ProcessLines(const Lines& lines);

	int LogFontSizeFromPointSize(int fontSize);
	int LogFontSizeToPointSize(int logFontSize);

	std::wstring GetSelectionInfoText(const std::wstring& label, const SelectionInfo& selection) const;
	SelectionInfo GetLogFileRange() const;
	void UpdateUI();
	void UpdateStatusBar();
	bool LoadSettings();
	void SaveSettings();

	bool IsPaused() const;
	void Pause();
	void Resume();
	bool GetAlwaysOnTop() const;
	void SetAlwaysOnTop(bool value);

	void AddFilterView();
	void AddFilterView(const std::wstring& name, const LogFilter& filter = LogFilter());
	void AddMessage(const Message& message);

	void SetModifiedMark(int tabindex, bool modified);
	void ClearLog();
	void SaveLogFile(const std::wstring& fileName);
	void SaveViewFile(const std::wstring& fileName);

	void OnContextMenu(HWND /*hWnd*/, CPoint pt);
	LRESULT OnSysCommand(UINT nCommand, CPoint);
	LRESULT OnSystemTrayIcon(UINT, WPARAM wParam, LPARAM lParam);
	LRESULT OnScRestore(UINT, INT, HWND);
	LRESULT OnScClose(UINT, INT, HWND);
	LRESULT OnBeginTabDrag(NMHDR* pnmh);
	LRESULT OnChangingTab(NMHDR* pnmh);
	LRESULT OnChangeTab(NMHDR* pnmh);
	LRESULT OnCloseTab(NMHDR* pnmh);
	LRESULT OnDeleteTab(NMHDR* pnmh);
	void OnFileNewTab(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnFileOpen(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnFileRun(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnFileSaveLog(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnFileExit(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnFileSaveView(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnFileLoadConfiguration(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnFileSaveConfiguration(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnLinkViews(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnAutoNewline(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnHide(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnAlwaysOnTop(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnLogClear(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnLogPause(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnLogGlobal(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnLogHistory(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnLogDebugviewAgent(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnViewFind(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnViewFont(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnViewFilter(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnViewClose(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnSources(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnAppAbout(UINT uNotifyCode, int nID, CWindow wndCtl);

	int GetViewCount() const;
	CLogView& GetView(int i);
	CLogView& GetView();
	void SetLogFont();
	void SetTitle(const std::wstring& title = L"");
	void HandleDroppedFile(const std::wstring& file);
	void Run(const std::wstring& pathName = L"");
	void AddLogSource(const SourceInfo& info);
	void CloseView(int i);

	LineBuffer m_lineBuffer;
	CCommandBarCtrl m_cmdBar;
	CMultiPaneStatusBarCtrl m_statusBar; // CMultiPaneStatusBarCtrlFlickerFree /  CMultiPaneStatusBarCtrl
	LogFile m_logFile;
	std::unique_ptr<FileWriter> m_logWriter;
	int m_filterNr;
	CFindDlg m_findDlg;
	Win32::HFont m_hFont;
	bool m_linkViews;
	bool m_hide;
	bool m_tryGlobal;
	CRunDlg m_runDlg;
	std::wstring m_logFileName;
	std::wstring m_txtFileName;
	std::wstring m_configFileName;
	size_t m_initialPrivateBytes;
	NOTIFYICONDATA m_notifyIconData;
	LOGFONT m_logfont;
	std::wstring m_applicationName;
	DBWinReader* m_pLocalReader;
	DBWinReader* m_pGlobalReader;
	DbgviewReader* m_pDbgviewReader;
	std::vector<SourceInfo> m_sourceInfos;
	std::unique_ptr<GuiExecutorClient> m_GuiExecutorClient;
	LogSources m_logSources;
};

} // namespace debugviewpp 
} // namespace fusion
