// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <algorithm>
#include <boost/utility.hpp>
#include <psapi.h>
#include "dbgstream.h"
#include "Utilities.h"
#include "Resource.h"
#include "FilterDlg.h"
#include "AboutDlg.h"
#include "LogView.h"
#include "MainFrame.h"
#include "Win32Lib.h"
#include "ProcessInfo.h"

#pragma comment(lib, "psapi.lib")

namespace fusion {

const unsigned int msOnTimerPeriod = 40;	// 25 frames/second intentionally near what the human eye can still perceive

void CLogViewTabItem::SetView(const std::shared_ptr<CLogView>& pView)
{
	m_pView = pView;
	SetTabView(*pView);
}

CLogView& CLogViewTabItem::GetView()
{
	return *m_pView;
}

BEGIN_MSG_MAP_TRY(CMainFrame)
	MSG_WM_CREATE(OnCreate)
	MSG_WM_CLOSE(OnClose)
	MSG_WM_TIMER(OnTimer)
	MSG_WM_SYSCOMMAND(OnSysCommand)
    MESSAGE_HANDLER_EX(WM_SYSTEMTRAYICON, OnSystemTrayIcon)
    COMMAND_ID_HANDLER_EX(SC_RESTORE, OnScRestore)
    COMMAND_ID_HANDLER_EX(SC_CLOSE, OnScClose)
	COMMAND_ID_HANDLER_EX(ID_FILE_NEWTAB, OnFileNewTab)
	COMMAND_ID_HANDLER_EX(ID_FILE_OPEN, OnFileOpen)
	COMMAND_ID_HANDLER_EX(ID_FILE_SAVE, OnFileSave)
	COMMAND_ID_HANDLER_EX(ID_FILE_SAVE_AS, OnFileSaveAs)
	COMMAND_ID_HANDLER_EX(ID_LOG_CLEAR, OnLogClear)
	COMMAND_ID_HANDLER_EX(ID_OPTIONS_AUTONEWLINE, OnAutoNewline)
	COMMAND_ID_HANDLER_EX(ID_OPTIONS_HIDE, OnHide)
	COMMAND_ID_HANDLER_EX(ID_LOG_PAUSE, OnLogPause)
	COMMAND_ID_HANDLER_EX(ID_LOG_GLOBAL, OnLogGlobal)
	COMMAND_ID_HANDLER_EX(ID_VIEW_FIND, OnViewFind)
	COMMAND_ID_HANDLER_EX(ID_OPTIONS_FONT, OnViewFont)
	COMMAND_ID_HANDLER_EX(ID_VIEW_FILTER, OnViewFilter)
	COMMAND_ID_HANDLER_EX(ID_APP_ABOUT, OnAppAbout)
	NOTIFY_CODE_HANDLER_EX(CTCN_BEGINITEMDRAG, OnBeginTabDrag)
	NOTIFY_CODE_HANDLER_EX(CTCN_SELCHANGING, OnChangingTab)
	NOTIFY_CODE_HANDLER_EX(CTCN_CLOSE, OnCloseTab)
	NOTIFY_CODE_HANDLER_EX(CTCN_DELETEITEM, OnDeleteTab);
	CHAIN_MSG_MAP(TabbedFrame)
	CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
	REFLECT_NOTIFICATIONS()
END_MSG_MAP_CATCH(ExceptionHandler)

LOGFONT& GetDefaultLogFont()
{
	static LOGFONT lf;
	static int initialized = GetObjectW(AtlGetDefaultGuiFont(), sizeof(lf), &lf);
	return lf;
}

CMainFrame::CMainFrame() :
	m_timeOffset(0),
	m_filterNr(0),
	m_findDlg(*this),
	m_fontDlg(&GetDefaultLogFont(), CF_SCREENFONTS | CF_NOVERTFONTS | CF_SELECTSCRIPT | CF_NOSCRIPTSEL),
	m_autoNewLine(false),
	m_hide(false),
	m_tryGlobal(true),
	m_initialPrivateBytes(ProcessInfo::GetPrivateBytes())
{
#ifdef CONSOLE_DEBUG
	AllocConsole();
	freopen_s(&m_stdout, "CONOUT$", "wb", stdout);
#endif

	m_notifyIconData.cbSize = 0;
	SetAutoNewLine(m_autoNewLine);
}

CMainFrame::~CMainFrame()
{
#ifdef CONSOLE_DEBUG
	fclose(m_stdout);
#endif
}

void CMainFrame::ExceptionHandler()
{
	MessageBox(WStr(GetExceptionMessage()), LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	if (GetActiveWindow() == m_findDlg)
	{
		if (m_findDlg.IsDialogMessage(pMsg))
			return TRUE;
	}

	return TabbedFrame::PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle()
{
	UpdateUI();
	UIUpdateToolBar();
	UIUpdateStatusBar();
	return FALSE;
}

LRESULT CMainFrame::OnCreate(const CREATESTRUCT* /*pCreate*/)
{
	m_notifyIconData.cbSize = 0;

	SetWindowText(WStr(LoadString(IDR_APPNAME)));

	m_findDlg.Create(*this, 0);

	CreateSimpleToolBar();
	UIAddToolBar(m_hWndToolBar);

	m_hWndStatusBar = m_statusBar.Create(*this);
	int paneIds[] = { ID_DEFAULT_PANE, ID_SELECTION_PANE, ID_VIEW_PANE, ID_LOGFILE_PANE, ID_MEMORY_PANE };
	m_statusBar.SetPanes(paneIds, 5, false);
	UIAddStatusBar(m_hWndStatusBar);

	CreateTabWindow(*this, rcDefault, CTCS_CLOSEBUTTON | CTCS_DRAGREARRANGE);

	GetTabCtrl().InsertItem(0, L"+");
	AddFilterView(L"View");
	HideTabControl();

	SetLogFont();
	try 
	{
		LoadSettings();
	}
	catch (std::exception e)
	{
		// handle bad registry entries?
	}

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != nullptr);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	m_timer = SetTimer(1, msOnTimerPeriod, nullptr);

	Resume(false);
	return 0;
}

void CMainFrame::OnClose()
{
	if (m_timer)
		KillTimer(m_timer); 

	SaveSettings();
	DestroyWindow();

	if (m_notifyIconData.cbSize)
	{
		Shell_NotifyIcon(NIM_DELETE, &m_notifyIconData);
		m_notifyIconData.cbSize = 0;
	}

#ifdef CONSOLE_DEBUG
	fclose(stdout);
	FreeConsole();
#endif
}

void CMainFrame::UpdateUI()
{
	UpdateStatusBar();

	UISetCheck(ID_VIEW_TIME, GetView().GetClockTime());
	UISetCheck(ID_VIEW_SCROLL, GetView().GetScroll());
	UISetCheck(ID_VIEW_BOOKMARK, GetView().GetBookmark());

	for (int id = ID_VIEW_COLUMN_FIRST; id <= ID_VIEW_COLUMN_LAST; ++id)
		UISetCheck(id, GetView().IsColumnViewed(id));

	UISetCheck(ID_OPTIONS_AUTONEWLINE, m_autoNewLine);
	UISetCheck(ID_OPTIONS_HIDE, m_hide);
	UISetCheck(ID_LOG_PAUSE, !m_pLocalReader);
	UIEnable(ID_LOG_GLOBAL, !!m_pLocalReader);
	UISetCheck(ID_LOG_GLOBAL, m_tryGlobal);
}

std::wstring FormatUnits(int n, const std::wstring& unit)
{
	if (n == 0)
		return L"";
	if (n == 1)
		return wstringbuilder() << n << " " << unit;
	return wstringbuilder() << n << " " << unit << "s";
}

std::wstring FormatDuration(double seconds)
{
	int minutes = floor_to<int>(seconds / 60);
	seconds -= 60 * minutes;

	int hours = minutes / 60;
	minutes -= 60 * hours;

	int days = hours / 24;
	hours -= 24 * days;

	if (days > 0)
		return wstringbuilder() << FormatUnits(days, L"day") << L" " << FormatUnits(hours, L"hour");

	if (hours > 0)
		return wstringbuilder() << FormatUnits(hours, L"hour") << L" " << FormatUnits(minutes, L"minute");

	if (minutes > 0)
		return wstringbuilder() << FormatUnits(minutes, L"minute") << L" " << FormatUnits(floor_to<int>(seconds), L"second");

	static const wchar_t* units[] = { L"s", L"ms", L"µs", L"ns", nullptr };
	const wchar_t** unit = units;
	while (*unit != nullptr && seconds > 0 && seconds < 1)
	{
		seconds *= 1e3;
		++unit;
	}

	return wstringbuilder() << std::fixed << std::setprecision(3) << seconds << L" " << *unit;
}

std::wstring FormatBytes(size_t size)
{
	static const wchar_t* units[] = { L"bytes", L"kB", L"MB", L"GB", L"TB", L"PB", L"EB", nullptr };
	const wchar_t** unit = units;
	const int kb = 1024;
	while (size / kb > 0 && unit[1] != nullptr)
	{
		size = size / kb;
		++unit;
	}

	return wstringbuilder() << size << L" " << *unit; 
}

std::wstring CMainFrame::GetSelectionInfoText(const std::wstring& label, const SelectionInfo& selection) const
{
	if (selection.count < 2)
		return std::wstring();

	double dt = m_logFile[selection.endLine].time - m_logFile[selection.beginLine].time;
	return wstringbuilder() << label << L": " << FormatDuration(dt) << L" (" << selection.count << " lines)";
}

SelectionInfo CMainFrame::GetLogFileRange() const
{
	if (m_logFile.Empty())
		return SelectionInfo();

	return SelectionInfo(0, m_logFile.Count() - 1, m_logFile.Count());
}

void CMainFrame::UpdateStatusBar()
{
	auto isearch = GetView().GetHighlightText();
	std::wstring search = wstringbuilder() << L"Searching: \"" << isearch << L"\"";
	UISetText(ID_DEFAULT_PANE,
		isearch.empty() ? (m_pLocalReader ? L"Ready" : L"Paused") : search.c_str());
	UISetText(ID_SELECTION_PANE, GetSelectionInfoText(L"Selected", GetView().GetSelectedRange()).c_str());
	UISetText(ID_VIEW_PANE, GetSelectionInfoText(L"View", GetView().GetViewRange()).c_str());
	UISetText(ID_LOGFILE_PANE, GetSelectionInfoText(L"Log", GetLogFileRange()).c_str());

	size_t memoryUsage = ProcessInfo::GetPrivateBytes() - m_initialPrivateBytes;
	if (memoryUsage < 0)
		memoryUsage = 0;
	UISetText(ID_MEMORY_PANE, FormatBytes(memoryUsage).c_str());
}

void CMainFrame::ProcessLines(const Lines& lines)
{
#ifdef CONSOLE_DEBUG
	if (lines.size() > 0)
		printf("incoming lines: %d\n", lines.size());
#endif

	if (m_logFile.Empty() && !lines.empty())
		m_timeOffset = lines[0].time;

	int views = GetViewCount();
	for (int i = 0; i < views; ++i)
		GetView(i).BeginUpdate();

	for (auto it = lines.begin(); it != lines.end(); ++it)
		AddMessage(Message(it->time - m_timeOffset, it->systemTime, it->pid, it->processName, it->message));

	for (int i = 0; i < views; ++i)
		GetView(i).EndUpdate();

	UpdateStatusBar();
}

void CMainFrame::OnTimer(UINT_PTR /*nIDEvent*/)
{
	Lines localLines;
	if (m_pLocalReader)
		localLines = m_pLocalReader->GetLines();

	Lines globalLines;
	if (m_pGlobalReader)
		globalLines = m_pGlobalReader->GetLines();

	Lines lines;
	lines.reserve(localLines.size() + globalLines.size());
	std::merge(localLines.begin(), localLines.end(), globalLines.begin(), globalLines.end(), std::back_inserter(lines), [](const Line& a, const Line& b) { return a.time < b.time; });
	ProcessLines(lines);
}

LRESULT CMainFrame::OnSysCommand(UINT nCommand, CPoint)
{
	switch (nCommand)
	{
	case SC_MINIMIZE:
		if (!m_hide)
			break;

		if (!m_notifyIconData.cbSize)
		{
			m_notifyIconData.cbSize = sizeof(m_notifyIconData);
			m_notifyIconData.hWnd = *this;
			m_notifyIconData.uID = 1;
			m_notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
			m_notifyIconData.uCallbackMessage = WM_SYSTEMTRAYICON;
			m_notifyIconData.hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
			CString sWindowText;
			GetWindowText(sWindowText);
			_tcscpy_s(m_notifyIconData.szTip, sWindowText);
			if (!Shell_NotifyIcon(NIM_ADD, &m_notifyIconData))
				break;
		}
		ShowWindow(SW_HIDE);
		return 0;
	}

	SetMsgHandled(false);
	return 0;
}

LRESULT CMainFrame::OnSystemTrayIcon(UINT, WPARAM wParam, LPARAM lParam)
{
	ATLASSERT(wParam == 1);
	wParam;
	switch (lParam)
	{
	case WM_LBUTTONDBLCLK:
		SendMessage(WM_COMMAND, SC_RESTORE);
		break;
	case WM_RBUTTONUP:
		{
			SetForegroundWindow(m_hWnd);
			CMenuHandle menu = GetSystemMenu(false);
			menu.EnableMenuItem(SC_RESTORE, MF_BYCOMMAND | MF_ENABLED);
			menu.EnableMenuItem(SC_MOVE, MF_BYCOMMAND | MF_GRAYED);
			menu.EnableMenuItem(SC_SIZE, MF_BYCOMMAND | MF_GRAYED);
			menu.EnableMenuItem(SC_MINIMIZE, MF_BYCOMMAND | MF_GRAYED);
			menu.EnableMenuItem(SC_MAXIMIZE, MF_BYCOMMAND | MF_GRAYED);
			menu.EnableMenuItem(SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
			POINT position;
			ATLVERIFY(GetCursorPos(&position));
			menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN, position.x, position.y, m_hWnd);
		}
		break;
	}
	return 0;
}

LRESULT CMainFrame::OnScRestore(UINT, INT, HWND)
{
	if (m_notifyIconData.cbSize)
	{
		Shell_NotifyIcon(NIM_DELETE, &m_notifyIconData);
		m_notifyIconData.cbSize = 0;
	}
	ShowWindow(SW_SHOW);
	BringWindowToTop();
	return 0;
}

LRESULT CMainFrame::OnScClose(UINT, INT, HWND)
{
	PostMessage(WM_COMMAND, IDCANCEL);
	return 0;
}

const wchar_t* RegistryPath = L"Software\\Fusion\\DebugView++";

bool CMainFrame::LoadSettings()
{
	DWORD x, y, cx, cy;
	CRegKey reg;
	if (reg.Open(HKEY_CURRENT_USER, RegistryPath, KEY_READ) != ERROR_SUCCESS)
		return false;
	reg.QueryDWORDValue(L"x", x);
	reg.QueryDWORDValue(L"y", y);
	reg.QueryDWORDValue(L"width", cx);
	reg.QueryDWORDValue(L"height", cy);
	SetWindowPos(0, x, y, cx, cy, SWP_NOZORDER);

	SetAutoNewLine(RegGetDWORDValue(reg, L"AutoNewLine", 1) != 0);
	m_hide = RegGetDWORDValue(reg, L"Hide", 0) != 0;

	auto fontName = RegGetStringValue(reg, L"FontName", L"").substr(0, LF_FACESIZE - 1);
	int fontSize = RegGetDWORDValue(reg, L"FontSize", 8);
	if (!fontName.empty())
	{
		LOGFONT lf;
		m_fontDlg.GetCurrentFont(&lf);
		std::copy(fontName.begin(), fontName.end(), lf.lfFaceName);
		lf.lfFaceName[fontName.size()] = '\0';
		lf.lfHeight = -MulDiv(fontSize, GetDeviceCaps(GetDC(), LOGPIXELSY), 72);
		m_fontDlg.SetLogFont(&lf);
		SetLogFont();
	}

	for (size_t i = 0; ; ++i)
	{
		CRegKey regView;
		if (regView.Open(reg, WStr(wstringbuilder() << L"Views\\View" << i)) != ERROR_SUCCESS)
			break;

		if (i > 0)
			AddFilterView(RegGetStringValue(regView));
		GetView().LoadSettings(regView);
	}

	CRegKey regColors;
	regColors.Open(reg, L"Colors");
	auto colors = ColorDialog::GetCustomColors();
	for (int i = 0; i < 16; ++i)
		colors[i] = RegGetDWORDValue(regColors, WStr(wstringbuilder() << L"Color" << i));

	return true;
}

void CMainFrame::SaveSettings()
{
	auto placement = fusion::GetWindowPlacement(*this);

	CRegKey reg;
	reg.Create(HKEY_CURRENT_USER, RegistryPath);
	reg.SetDWORDValue(L"x", placement.rcNormalPosition.left);
	reg.SetDWORDValue(L"y", placement.rcNormalPosition.top);
	reg.SetDWORDValue(L"width", placement.rcNormalPosition.right - placement.rcNormalPosition.left);
	reg.SetDWORDValue(L"height", placement.rcNormalPosition.bottom - placement.rcNormalPosition.top);

	reg.SetDWORDValue(L"AutoNewLine", m_autoNewLine);
	reg.SetDWORDValue(L"Hide", m_hide);

	LOGFONT lf;
	m_fontDlg.GetCurrentFont(&lf);
	reg.SetStringValue(L"FontName", lf.lfFaceName);
	reg.SetDWORDValue(L"FontSize", -MulDiv(lf.lfHeight, 72, GetDeviceCaps(GetDC(), LOGPIXELSY)));

	reg.RecurseDeleteKey(L"Views");
	int views = GetViewCount();
	for (int i = 0; i < views; ++i)
	{
		CRegKey regView;
		regView.Create(reg, WStr(wstringbuilder() << L"Views\\View" << i));
		regView.SetStringValue(L"", GetTabCtrl().GetItem(i)->GetTextRef());
		GetView(i).SaveSettings(regView);
	}

	CRegKey regColors;
	regColors.Create(reg, L"Colors");
	auto colors = ColorDialog::GetCustomColors();
	for (int i = 0; i < 16; ++i)
		regColors.SetDWORDValue(WStr(wstringbuilder() << L"Color" << i), colors[i]);
}

bool CMainFrame::GetAutoNewLine() const
{
	return m_autoNewLine;
}

void CMainFrame::SetAutoNewLine(bool value)
{
	if (m_pLocalReader)
		m_pLocalReader->AutoNewLine(value);
	if (m_pGlobalReader)
		m_pGlobalReader->AutoNewLine(value);
	m_autoNewLine = value;
}

void CMainFrame::FindNext(const std::wstring& text)
{
	if (!GetView().FindNext(text))
		MessageBeep(MB_ICONASTERISK);
}

void CMainFrame::FindPrevious(const std::wstring& text)
{
	if (!GetView().FindPrevious(text))
		MessageBeep(MB_ICONASTERISK);
}

void CMainFrame::AddFilterView()
{
	++m_filterNr;
	CFilterDlg dlg(wstringbuilder() << L"Filter " << m_filterNr);
	if (dlg.DoModal() != IDOK)
		return;

	AddFilterView(dlg.GetName(), dlg.GetFilters());
}

void CMainFrame::AddFilterView(const std::wstring& name, const LogFilter& filter)
{
	auto pView = std::make_shared<CLogView>(*this, m_logFile, filter);
	pView->Create(*this, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);
	pView->SetFont(m_hFont.get());

	int newIndex = GetTabCtrl().GetItemCount() - 1;
	GetTabCtrl().InsertItem(newIndex, name.c_str());
	GetTabCtrl().GetItem(newIndex)->SetView(pView);
	GetTabCtrl().SetCurSel(newIndex);
	ShowTabControl();
}

LRESULT CMainFrame::OnBeginTabDrag(NMHDR* pnmh)
{
	auto& nmhdr = *reinterpret_cast<NMCTCITEM*>(pnmh);

	return nmhdr.iItem >= GetViewCount();
}

LRESULT CMainFrame::OnChangingTab(NMHDR* pnmh)
{
	auto& nmhdr = *reinterpret_cast<NMCTC2ITEMS*>(pnmh);

	// The TabCtrl doesn't like the FiltersDialog during its message processing.
	// The PostMessage triggers the new tab after the TabControl message handling
	if (nmhdr.iItem2 > 0 && nmhdr.iItem2 == GetViewCount())
		PostMessage(WM_COMMAND, ID_FILE_NEWTAB, (LPARAM)m_hWnd);

	return FALSE;
}

LRESULT CMainFrame::OnCloseTab(NMHDR* pnmh)
{
	auto& nmhdr = *reinterpret_cast<NMCTCITEM*>(pnmh);

	int filterIndex = nmhdr.iItem;
	int views = GetViewCount();
	if (filterIndex >= 0 && filterIndex < views)
	{
		GetTabCtrl().DeleteItem(filterIndex, false);
		int select = filterIndex == views - 1 ? filterIndex - 1 : filterIndex;
		GetTabCtrl().SetCurSel(select);
		if (GetViewCount() == 1)
			HideTabControl();
	}
	return 0;
}

LRESULT CMainFrame::OnDeleteTab(NMHDR* pnmh)
{
	auto& nmhdr = *reinterpret_cast<NMCTCITEM*>(pnmh);

	if (nmhdr.iItem >= 0 && nmhdr.iItem < GetViewCount())
		GetView(nmhdr.iItem).DestroyWindow();

	return FALSE;
}

void CMainFrame::OnFileNewTab(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	AddFilterView();
}

std::wstring CMainFrame::GetLogFileName() const
{
	std::wstring fileName = !m_logFileName.empty() ? m_logFileName : L"DebugView.txt";
	CFileDialog dlg(false, L".txt", fileName.c_str(), OFN_OVERWRITEPROMPT, L"Text Files (*.txt)\0*.txt\0All Files\0*.*\0\0", 0);
	dlg.m_ofn.nFilterIndex = 0;
	dlg.m_ofn.lpstrTitle = L"Save DebugView log";
	return dlg.DoModal() == IDOK ? dlg.m_szFileName : L"";
}

void CMainFrame::SaveLogFile(const std::wstring& fileName)
{
	UISetText(0, WStr(wstringbuilder() << "Saving " << fileName));
	ScopedCursor cursor(::LoadCursor(nullptr, IDC_WAIT));
	GetView().Save(fileName);
	m_logFileName = fileName;
	UpdateStatusBar();
}

class TabSplitter
{
public:
	explicit TabSplitter(const std::string& text);

	std::string GetNext();
	std::string GetTail() const;

private:
	std::string::const_iterator m_it;
	std::string::const_iterator m_end;
};

TabSplitter::TabSplitter(const std::string& text) :
	m_it(text.begin()),
	m_end(text.end())
{
};

std::string TabSplitter::GetNext()
{
	auto it = std::find(m_it, m_end, '\t');
	std::string s(m_it, it);
	m_it = it == m_end ? it : it + 1;
	return s;
}

std::string TabSplitter::GetTail() const
{
	return std::string(m_it, m_end);
}

void CMainFrame::OnFileOpen(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	std::wstring fileName = !m_logFileName.empty() ? m_logFileName : L"DebugView.txt";
	CFileDialog dlg(true, L".txt", fileName.c_str(), OFN_FILEMUSTEXIST, L"Text Files (*.txt)\0*.txt\0All Files\0*.*\0\0", 0);
	dlg.m_ofn.nFilterIndex = 0;
	dlg.m_ofn.lpstrTitle = L"Load DebugView log";
	if (dlg.DoModal() != IDOK)
		return;

	fileName = dlg.m_szFileName;

	std::ifstream file(fileName);
	if (!file)
		ThrowLastError(fileName);

	Pause();
	ClearLog();

	DWORD pid = GetCurrentProcessId();
	std::string line;
	while (std::getline(file, line))
	{
		TabSplitter split(line);
		auto lineno = split.GetNext();
		auto time = split.GetNext();
		auto pidtxt = split.GetNext();
		auto process = split.GetNext();
		auto message = split.GetTail();

		FILETIME ft = {};
		AddMessage(Message(0, ft, pid, process, message));
	}
}

void CMainFrame::OnFileSave(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	auto fileName = !m_logFileName.empty() ? m_logFileName : GetLogFileName();
	if (!fileName.empty())
		SaveLogFile(fileName);
}

void CMainFrame::OnFileSaveAs(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	auto fileName = GetLogFileName();
	if (!fileName.empty())
		SaveLogFile(fileName);
}

void CMainFrame::ClearLog()
{
	int views = GetViewCount();
	for (int i = 0; i < views; ++i)
		GetView(i).Clear();
	m_logFile.Clear();
}

void CMainFrame::OnLogClear(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	ClearLog();
}

void CMainFrame::OnAutoNewline(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	SetAutoNewLine(!GetAutoNewLine());
}

void CMainFrame::OnHide(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	m_hide = !m_hide;
}

bool CMainFrame::IsPaused() const
{
	return !m_pLocalReader;
}

void CMainFrame::Pause()
{
	m_pLocalReader.reset();
	m_pGlobalReader.reset();
}

void CMainFrame::Resume(bool report = true)
{
	if (!m_pLocalReader)
		m_pLocalReader = make_unique<DBWinReader>(false);

	if (m_tryGlobal)
	{
		try
		{
			m_pGlobalReader = make_unique<DBWinReader>(true);
		}
		catch (std::exception&)
		{
			if (report)
			{
				MessageBox(
					L"Unable to capture Global Win32 Messages.\n"
					L"\n"
					L"Make sure you have appropriate permissions.\n"
					L"\n"
					L"You may need to start this application by right-clicking it and selecting\n"
					L"'Run As Administator' even if you have administrator rights.",
					LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
			}
			m_tryGlobal = false;
		}
	}

	SetAutoNewLine(GetAutoNewLine());
}

void CMainFrame::OnLogPause(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	if (IsPaused())
		Resume(true);
	else
		Pause();
}

void CMainFrame::OnLogGlobal(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	m_tryGlobal = !m_pGlobalReader;
	if (m_pLocalReader && m_tryGlobal)
		Resume(true);
	else
		m_pGlobalReader.reset();
}

void CMainFrame::OnViewFilter(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	int tabIdx = GetTabCtrl().GetCurSel();

	CFilterDlg dlg(GetTabCtrl().GetItem(tabIdx)->GetTextRef(), GetView().GetFilters());
	if (dlg.DoModal() != IDOK)
		return;

	GetTabCtrl().GetItem(tabIdx)->SetText(dlg.GetName().c_str());
	GetTabCtrl().UpdateLayout();
	GetTabCtrl().Invalidate();
	GetView().SetFilters(dlg.GetFilters());
}

void CMainFrame::OnViewFind(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	m_findDlg.Show();
}

void CMainFrame::OnViewFont(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	if (m_fontDlg.DoModal(*this) == IDOK)
		SetLogFont();
}

void CMainFrame::SetLogFont()
{
	LOGFONT lf;
	m_fontDlg.GetCurrentFont(&lf);
	HFont hFont(CreateFontIndirect(&lf));
	if (!hFont)
		return;

	int views = GetViewCount();
	for (int i = 0; i < views; ++i)
		GetView(i).SetFont(hFont.get());
	m_hFont = std::move(hFont);
}

void CMainFrame::OnAppAbout(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
}

int CMainFrame::GetViewCount() const
{
	return const_cast<CMainFrame&>(*this).GetTabCtrl().GetItemCount() - 1;
}

CLogView& CMainFrame::GetView(int i)
{
	return GetTabCtrl().GetItem(i)->GetView();
}

CLogView& CMainFrame::GetView()
{
	assert(GetViewCount() > 0);

	int i = GetTabCtrl().GetCurSel();
	if (i < 0 || i >= GetTabCtrl().GetItemCount() - 1)
		i = 0;

	return GetView(i);
}

bool CMainFrame::IsDbgViewClearMessage(const std::string& text) const
{
	return text.find("DBGVIEWCLEAR") != std::string::npos;
}

void CMainFrame::AddMessage(const Message& message)
{
	if (IsDbgViewClearMessage(message.text))
	{
		OnLogClear(0, 0, 0);
		return;
	}

	int index = m_logFile.Count();
	m_logFile.Add(message);
	int views = GetViewCount();
	for (int i = 0; i < views; ++i)
		GetView(i).Add(index, message);
}

} // namespace fusion
