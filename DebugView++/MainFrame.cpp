// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <algorithm>
#include <boost/utility.hpp>
#include <boost/filesystem.hpp>
#include "dbgstream.h"
#include "hstream.h"
#include "Process.h"
#include "PipeReader.h"
#include "ProcessReader.h"
#include "FileReader.h"
#include "Utilities.h"
#include "Resource.h"
#include "RunDlg.h"
#include "FilterDlg.h"
#include "AboutDlg.h"
#include "LogView.h"
#include "MainFrame.h"
#include "Win32Lib.h"
#include "ProcessInfo.h"

namespace fusion {
namespace debugviewpp {

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
	MSG_WM_DROPFILES(OnDropFiles)
	MSG_WM_SYSCOMMAND(OnSysCommand)
	MESSAGE_HANDLER_EX(WM_SYSTEMTRAYICON, OnSystemTrayIcon)
	COMMAND_ID_HANDLER_EX(SC_RESTORE, OnScRestore)
	COMMAND_ID_HANDLER_EX(SC_CLOSE, OnScClose)
	COMMAND_ID_HANDLER_EX(ID_FILE_NEWVIEW, OnFileNewTab)
	COMMAND_ID_HANDLER_EX(ID_FILE_OPEN, OnFileOpen)
	COMMAND_ID_HANDLER_EX(ID_FILE_RUN, OnFileRun)
	COMMAND_ID_HANDLER_EX(ID_FILE_SAVE_LOG, OnFileSaveLog)
	COMMAND_ID_HANDLER_EX(ID_FILE_SAVE_VIEW, OnFileSaveView)
	COMMAND_ID_HANDLER_EX(ID_LOG_CLEAR, OnLogClear)
	COMMAND_ID_HANDLER_EX(ID_LOG_PAUSE, OnLogPause)
	COMMAND_ID_HANDLER_EX(ID_LOG_GLOBAL, OnLogGlobal)
	COMMAND_ID_HANDLER_EX(ID_VIEW_FIND, OnViewFind)
	COMMAND_ID_HANDLER_EX(ID_VIEW_FILTER, OnViewFilter)
	COMMAND_ID_HANDLER_EX(ID_OPTIONS_LINKVIEWS, OnLinkViews)
	COMMAND_ID_HANDLER_EX(ID_OPTIONS_AUTONEWLINE, OnAutoNewline)
	COMMAND_ID_HANDLER_EX(ID_OPTIONS_FONT, OnViewFont)
	COMMAND_ID_HANDLER_EX(ID_OPTIONS_ALWAYSONTOP, OnAlwaysOnTop)
	COMMAND_ID_HANDLER_EX(ID_OPTIONS_HIDE, OnHide)
	COMMAND_ID_HANDLER_EX(ID_APP_ABOUT, OnAppAbout)
	NOTIFY_CODE_HANDLER_EX(CTCN_BEGINITEMDRAG, OnBeginTabDrag)
	NOTIFY_CODE_HANDLER_EX(CTCN_SELCHANGING, OnChangingTab)
	NOTIFY_CODE_HANDLER_EX(CTCN_SELCHANGE, OnChangeTab)
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
	m_linkViews(false),
	m_autoNewLine(false),
	m_hide(false),
	m_tryGlobal(HasGlobalDBWinReaderRights()),
	m_initialPrivateBytes(ProcessInfo::GetPrivateBytes()),
	m_logfont(GetDefaultLogFont())
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
	MessageBox(WStr(GetExceptionMessage()).c_str(), LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
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

	HWND hWndCmdBar = m_cmdBar.Create(*this, rcDefault, nullptr, ATL_SIMPLE_CMDBAR_PANE_STYLE);
	m_cmdBar.AttachMenu(GetMenu());
	m_cmdBar.LoadImages(IDR_MAINFRAME);
	SetMenu(nullptr);

	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	CReBarCtrl rebar(m_hWndToolBar);

	AddSimpleReBarBand(hWndCmdBar);

	HWND hWndToolBar = CreateSimpleToolBarCtrl(rebar, IDR_MAINFRAME, false, ATL_SIMPLE_TOOLBAR_PANE_STYLE);
	AddSimpleReBarBand(hWndToolBar, nullptr, true);
	UIAddToolBar(hWndToolBar);

	m_findDlg.Create(rebar);
	AddSimpleReBarBand(m_findDlg, L"Find: ", false, 10000);
	SizeSimpleReBarBands();

	rebar.LockBands(true);
	rebar.SetNotifyWnd(*this);

	m_hWndStatusBar = m_statusBar.Create(*this);
	int paneIds[] = { ID_DEFAULT_PANE, ID_SELECTION_PANE, ID_VIEW_PANE, ID_LOGFILE_PANE, ID_MEMORY_PANE };
	m_statusBar.SetPanes(paneIds, 5, false);
	UIAddStatusBar(m_hWndStatusBar);

	CreateTabWindow(*this, rcDefault, CTCS_CLOSEBUTTON | CTCS_DRAGREARRANGE);

	GetTabCtrl().InsertItem(0, L"+");
	AddFilterView(L"View");
	HideTabControl();

	SetLogFont();
	LoadSettings();

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != nullptr);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	m_timer = SetTimer(1, msOnTimerPeriod, nullptr);
	DragAcceptFiles(true);

	// Resume can throw if a second debugview is running
	// so do not rely on any commands executed afterwards
	if (!IsDBWinViewerActive())
		Resume();
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
	UISetCheck(ID_VIEW_PROCESSCOLORS, GetView().GetViewProcessColors());
	UISetCheck(ID_VIEW_SCROLL, GetView().GetScroll());
	UISetCheck(ID_VIEW_BOOKMARK, GetView().GetBookmark());

	for (int id = ID_VIEW_COLUMN_FIRST; id <= ID_VIEW_COLUMN_LAST; ++id)
		UISetCheck(id, GetView().IsColumnViewed(id));

	UISetCheck(ID_OPTIONS_LINKVIEWS, m_linkViews);
	UISetCheck(ID_OPTIONS_AUTONEWLINE, m_autoNewLine);
	UISetCheck(ID_OPTIONS_ALWAYSONTOP, GetAlwaysOnTop());
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

std::wstring FormatDateTime(const SYSTEMTIME& systemTime)
{
	int size = GetTimeFormat(LOCALE_USER_DEFAULT, 0, &systemTime, nullptr, nullptr, 0);
	size += GetDateFormat(LOCALE_USER_DEFAULT, 0, &systemTime, nullptr, nullptr, 0);
	std::vector<wchar_t> buf(size);

	int offset = GetDateFormat(LOCALE_USER_DEFAULT, 0, &systemTime, nullptr, buf.data(), size);
	buf[offset - 1] = ' ';
	GetTimeFormat(LOCALE_USER_DEFAULT, 0, &systemTime, nullptr, buf.data() + offset, size);
	return std::wstring(buf.data(), size - 1);
}

std::wstring FormatDateTime(const FILETIME& fileTime)
{
	return FormatDateTime(FileTimeToSystemTime(FileTimeToLocalFileTime(fileTime)));
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
	if (selection.count == 0)
		return std::wstring();

	if (selection.count == 1)
		return label + L": " + FormatDateTime(m_logFile[selection.beginLine].systemTime);

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
	if (m_logFile.Empty() && !lines.empty())
		m_timeOffset = lines[0].time;

	int views = GetViewCount();
	for (int i = 0; i < views; ++i)
		GetView(i).BeginUpdate();

	for (auto it = lines.begin(); it != lines.end(); ++it)
		AddMessage(Message(it->time - m_timeOffset, it->systemTime, it->pid, it->processName, it->message));

	for (int i = 0; i < views; ++i)
	{
		if (GetView(i).EndUpdate() > 0 && GetTabCtrl().GetCurSel() != i)
		{
//			GetTabCtrl().GetItem(i)->SetHighlighted(true);
			GetTabCtrl().GetItem(i)->SetText((GetView(i).GetName() + L"*").c_str());
			GetTabCtrl().UpdateLayout();
			GetTabCtrl().Invalidate();
		}
	}

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
	auto pred = [](const Line& a, const Line& b) { return a.time < b.time; };
	std::merge(localLines.begin(), localLines.end(), globalLines.begin(), globalLines.end(), std::back_inserter(lines), pred);
	if (m_pSources.empty())
		return ProcessLines(lines);

	for (auto it = m_pSources.begin(); it != m_pSources.end(); )
	{
		Lines pipeLines((*it)->GetLines());
		Lines lines2;
		lines2.reserve(lines.size() + pipeLines.size());
		std::merge(lines.begin(), lines.end(), pipeLines.begin(), pipeLines.end(), std::back_inserter(lines2), pred);
		lines.swap(lines2);

		if ((*it)->AtEnd())
			it = m_pSources.erase(it);
		else
			++it;
	}

	ProcessLines(lines);
}

void CMainFrame::HandleDroppedFile(const std::wstring& file)
{
	using boost::algorithm::iequals;
	std::wstring ext = boost::filesystem::wpath(file).extension().wstring();

	if (iequals(ext, L".dblog") || iequals(ext, L".log"))
	{
		AddDBLogReader(file);
	}
	else if (iequals(ext, L".exe"))
	{
		cdbg << "Started capturing output of " << Str(file) << "\n";
		Run(file);
	}
	else if (iequals(ext, L".cmd") || iequals(ext, L".bat"))
	{
		cdbg << "Started capturing output of " << Str(file) << "\n";
		AddProcessReader(L"cmd.exe", wstringbuilder() << L"/Q /C " << file);
	}
	else
	{
		cdbg << "Started tailing " << Str(file) << "\n";
		AddFileReader(file);
	}
}

void CMainFrame::OnDropFiles(HDROP hDropInfo)
{
	auto guard = make_guard([hDropInfo]() { DragFinish(hDropInfo); });

	if (DragQueryFile(hDropInfo, 0xFFFFFFFF, nullptr, 0) == 1)
	{
		std::vector<wchar_t> fileName(DragQueryFile(hDropInfo, 0, nullptr, 0) + 1);
		if (DragQueryFile(hDropInfo, 0, fileName.data(), fileName.size()))
			HandleDroppedFile(std::wstring(fileName.data()));
	}
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
			POINT position = GetCursorPos();
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

const wchar_t* RegistryPath = L"Software\\Cobalt Fusion\\DebugView++";

bool CMainFrame::LoadSettings()
{
	DWORD x, y, cx, cy;
	CRegKey reg;
	reg.Create(HKEY_CURRENT_USER, RegistryPath);
	if (reg.QueryDWORDValue(L"X", x) == ERROR_SUCCESS && static_cast<int>(x) >= GetSystemMetrics(SM_XVIRTUALSCREEN) &&
		reg.QueryDWORDValue(L"Y", y) == ERROR_SUCCESS && static_cast<int>(y) >= GetSystemMetrics(SM_YVIRTUALSCREEN) &&
		reg.QueryDWORDValue(L"Width", cx) == ERROR_SUCCESS && static_cast<int>(x + cx) <= GetSystemMetrics(SM_CXVIRTUALSCREEN) &&
		reg.QueryDWORDValue(L"Height", cy) == ERROR_SUCCESS && static_cast<int>(y + cy) <= GetSystemMetrics(SM_CYVIRTUALSCREEN))
		SetWindowPos(0, x, y, cx, cy, SWP_NOZORDER);

	m_linkViews = RegGetDWORDValue(reg, L"LinkViews", 0) != 0;
	SetAutoNewLine(RegGetDWORDValue(reg, L"AutoNewLine", 1) != 0);
	SetAlwaysOnTop(RegGetDWORDValue(reg, L"AlwaysOnTop", 0) != 0);

	m_applicationName = RegGetStringValue(reg, L"ApplicationName", L"DebugView++");
	SetTitle();

	m_hide = RegGetDWORDValue(reg, L"Hide", 0) != 0;

	auto fontName = RegGetStringValue(reg, L"FontName", L"").substr(0, LF_FACESIZE - 1);
	int fontSize = RegGetDWORDValue(reg, L"FontSize", 8);
	if (!fontName.empty())
	{
		LOGFONT lf = {0};
		m_logfont = lf;
		std::copy(fontName.begin(), fontName.end(), m_logfont.lfFaceName);
		m_logfont.lfHeight = -MulDiv(fontSize, GetDeviceCaps(GetDC(), LOGPIXELSY), 72);
		SetLogFont();
	}

	CRegKey regViews;
	if (regViews.Open(reg, L"Views") == ERROR_SUCCESS)
	{
		for (size_t i = 0; ; ++i)
		{
			CRegKey regView;
			if (regView.Open(regViews, WStr(wstringbuilder() << L"View" << i)) != ERROR_SUCCESS)
				break;

			auto name = RegGetStringValue(regView);
			if (i == 0)
				GetTabCtrl().GetItem(0)->SetText(name.c_str());
			else
				AddFilterView(name);
			GetView().LoadSettings(regView);
		}
		GetTabCtrl().SetCurSel(RegGetDWORDValue(regViews, L"Current", 0));
		GetTabCtrl().UpdateLayout();
		GetTabCtrl().Invalidate();
	}

	CRegKey regColors;
	if (regColors.Open(reg, L"Colors") == ERROR_SUCCESS)
	{
		auto colors = ColorDialog::GetCustomColors();
		for (int i = 0; i < 16; ++i)
			colors[i] = RegGetDWORDValue(regColors, WStr(wstringbuilder() << L"Color" << i));
	}

	return true;
}

void CMainFrame::SaveSettings()
{
	auto placement = fusion::GetWindowPlacement(*this);

	CRegKey reg;
	reg.Create(HKEY_CURRENT_USER, RegistryPath);
	reg.SetDWORDValue(L"X", placement.rcNormalPosition.left);
	reg.SetDWORDValue(L"Y", placement.rcNormalPosition.top);
	reg.SetDWORDValue(L"Width", placement.rcNormalPosition.right - placement.rcNormalPosition.left);
	reg.SetDWORDValue(L"Height", placement.rcNormalPosition.bottom - placement.rcNormalPosition.top);

	reg.SetDWORDValue(L"LinkViews", m_linkViews);
	reg.SetDWORDValue(L"AutoNewLine", m_autoNewLine);
	reg.SetDWORDValue(L"AlwaysOnTop", GetAlwaysOnTop());
	reg.SetDWORDValue(L"Hide", m_hide);

	reg.SetStringValue(L"FontName", m_logfont.lfFaceName);
	reg.SetDWORDValue(L"FontSize", -MulDiv(m_logfont.lfHeight, 72, GetDeviceCaps(GetDC(), LOGPIXELSY)));

	reg.RecurseDeleteKey(L"Views");

	CRegKey regViews;
	regViews.Create(reg, L"Views");
	regViews.SetDWORDValue(L"Current", GetTabCtrl().GetCurSel());

	int views = GetViewCount();
	for (int i = 0; i < views; ++i)
	{
		CRegKey regView;
		regView.Create(regViews, WStr(wstringbuilder() << L"View" << i));
		regView.SetStringValue(L"", GetView(i).GetName().c_str());
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
	auto pView = std::make_shared<CLogView>(name, *this, m_logFile, filter);
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
		PostMessage(WM_COMMAND, ID_FILE_NEWVIEW, (LPARAM)m_hWnd);

	return FALSE;
}

LRESULT CMainFrame::OnChangeTab(NMHDR* pnmh)
{
	SetMsgHandled(false);

	auto& nmhdr = *reinterpret_cast<NMCTC2ITEMS*>(pnmh);

	if (nmhdr.iItem2 >= 0 && nmhdr.iItem2 < GetViewCount())
	{
//		GetTabCtrl().GetItem(nmhdr.iItem2)->SetHighlighted(false);
		GetTabCtrl().GetItem(nmhdr.iItem2)->SetText(GetView(nmhdr.iItem2).GetName().c_str());
	}

	if (!m_linkViews || nmhdr.iItem1 == nmhdr.iItem2 ||
		nmhdr.iItem1 < 0 || nmhdr.iItem1 >= GetViewCount() ||
		nmhdr.iItem2 < 0 || nmhdr.iItem2 >= GetViewCount())
		return 0;

	int line = GetView(nmhdr.iItem1).GetFocusLine();
	GetView(nmhdr.iItem2).SetFocusLine(line);
	
	return 0;
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

std::ostream& operator<<(std::ostream& os, const FILETIME& ft)
{
	uint64_t hi = ft.dwHighDateTime;
	uint64_t lo = ft.dwLowDateTime;
	return os << ((hi << 32) | lo);
}

void CMainFrame::SaveLogFile(const std::wstring& fileName)
{
	UISetText(0, WStr(wstringbuilder() << "Saving " << fileName));
	ScopedCursor cursor(::LoadCursor(nullptr, IDC_WAIT));

	std::ofstream fs(fileName);
	int count = m_logFile.Count();
	for (int i = 0; i < count; ++i)
	{
		auto msg = m_logFile[i];
		fs <<
			msg.time << '\t' <<
			msg.systemTime << '\t'<<
			msg.processId << '\t'<<
			msg.processName << '\t'<<
			msg.text << '\n';
	}
	fs.close();
	if (!fs)
		ThrowLastError(fileName);

	m_logFileName = fileName;
	UpdateStatusBar();
}

void CMainFrame::SaveViewFile(const std::wstring& fileName)
{
	UISetText(0, WStr(wstringbuilder() << "Saving " << fileName));
	ScopedCursor cursor(::LoadCursor(nullptr, IDC_WAIT));
	GetView().Save(fileName);
	m_txtFileName = fileName;
	UpdateStatusBar();
}

void CMainFrame::OnFileOpen(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	std::wstring fileName = !m_logFileName.empty() ? m_logFileName : L"DebugView++.dblog";
	CFileDialog dlg(true, L".dblog", fileName.c_str(), OFN_FILEMUSTEXIST,
		L"DebugView++ Log Files (*.dblog)\0*.dblog\0"
		L"DebugView Log Files (*.log)\0*.log\0"
		L"All Files (*.*)\0*.*\0\0",
		0);
	dlg.m_ofn.nFilterIndex = 0;
	dlg.m_ofn.lpstrTitle = L"Load Log File";
	if (dlg.DoModal() == IDOK)
		Load(std::wstring(dlg.m_szFileName));
}

void CMainFrame::AddProcessReader(const std::wstring& pathName, const std::wstring& args)
{
	m_pSources.push_back(make_unique<ProcessReader>(pathName, args));
}

void CMainFrame::AddFileReader(const std::wstring& filename)
{
	m_pSources.push_back(make_unique<FileReader>(filename));
}

void CMainFrame::AddDBLogReader(const std::wstring& filename)
{
	m_pSources.push_back(make_unique<DBLogReader>(filename));
}


void CMainFrame::Run(const std::wstring& pathName)
{
	if (!pathName.empty())
		m_runDlg.SetPathName(pathName);

	if (m_runDlg.DoModal() == IDOK)
		AddProcessReader(m_runDlg.GetPathName(), m_runDlg.GetArguments());
}

void CMainFrame::OnFileRun(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	Run();
}

void CMainFrame::Load(const std::wstring& fileName)
{
	std::ifstream file(fileName);
	if (!file)
		ThrowLastError(fileName);

	WIN32_FILE_ATTRIBUTE_DATA fileInfo = { 0 };
	GetFileAttributesEx(fileName.c_str(), GetFileExInfoStandard, &fileInfo);
	Load(file, boost::filesystem::wpath(fileName).filename().string(), fileInfo.ftCreationTime);
	SetTitle(fileName);
}

void CMainFrame::SetTitle(const std::wstring& title)
{
	std::wstring windowText = title.empty() ? m_applicationName : L"[" + title + L"] - " + m_applicationName;
	SetWindowText(windowText.c_str());
}

void CMainFrame::Load(HANDLE hFile)
{
	hstream file(hFile);
	FILETIME ft = { 0 };
	Load(file, "", ft);
}

void CMainFrame::Load(std::istream& file, const std::string& name, FILETIME fileTime)
{
	ScopedCursor cursor(::LoadCursor(nullptr, IDC_WAIT));

	Pause();
	ClearLog();

	std::string line;
	while (std::getline(file, line))
	{
		TabSplitter split(line);
		auto col1 = split.GetNext();
		auto col2 = split.GetNext();
		auto col3 = split.GetNext();
		if (!col3.empty() && col3[0] == '[')
		{
			std::istringstream is2(col2);
			std::istringstream is3(col3);
			DWORD pid;
			char c1, c2, c3;
			double time;
			std::string msg;
			if (is2 >> time && is3 >> std::noskipws >> c1 >> pid >> c2 >> c3 && c1 == '[' && c2 == ']' && c3 == ' ' && std::getline(is3, msg))
				AddMessage(Message(time, fileTime, pid, name, msg));
		}
		else
		{
			auto time = boost::lexical_cast<double>(col1);
			auto systemTime = MakeFileTime(boost::lexical_cast<uint64_t>(col2));
			auto pid = boost::lexical_cast<DWORD>(col3);
			auto process = split.GetNext();
			auto message = split.GetTail();

			AddMessage(Message(time, systemTime, pid, process, message));
		}
	}
}

void CMainFrame::CapturePipe(HANDLE hPipe)
{
	DWORD pid = GetParentProcessId();
	m_pSources.push_back(make_unique<PipeReader>(hPipe, pid, Str(ProcessInfo::GetProcessNameByPid(pid)).str()));
}

void CMainFrame::OnFileSaveLog(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	std::wstring fileName = !m_logFileName.empty() ? m_logFileName : L"DebugView++.dblog";
	CFileDialog dlg(false, L".dblog", fileName.c_str(), OFN_OVERWRITEPROMPT,
		L"DebugView++ Log Files (*.dblog)\0*.dblog\0"
		L"All Files (*.*)\0*.*\0\0", 0);
	dlg.m_ofn.nFilterIndex = 0;
	dlg.m_ofn.lpstrTitle = L"Save DebugView++ Log";
	if (dlg.DoModal() == IDOK)
		SaveLogFile(dlg.m_szFileName);
}

void CMainFrame::OnFileSaveView(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	std::wstring fileName = !m_txtFileName.empty() ? m_txtFileName : L"DebugView.txt";
	CFileDialog dlg(false, L".txt", fileName.c_str(), OFN_OVERWRITEPROMPT, L"Text Files (*.txt)\0*.txt\0All Files\0*.*\0\0", 0);
	dlg.m_ofn.nFilterIndex = 0;
	dlg.m_ofn.lpstrTitle = L"Save DebugView text";
	if (dlg.DoModal() == IDOK)
		SaveViewFile(fileName);
}

void CMainFrame::ClearLog()
{
	// First Clear LogFile such that views reset their m_firstLine:
	m_logFile.Clear();
	int views = GetViewCount();
	for (int i = 0; i < views; ++i)
		GetView(i).Clear();
}

void CMainFrame::OnLogClear(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	ClearLog();
}

void CMainFrame::OnLinkViews(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	m_linkViews = !m_linkViews;
}

void CMainFrame::OnAutoNewline(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	SetAutoNewLine(!GetAutoNewLine());
}

void CMainFrame::OnHide(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	m_hide = !m_hide;
}

bool CMainFrame::GetAlwaysOnTop() const
{
	return (GetWindowLong(GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
}

void CMainFrame::SetAlwaysOnTop(bool value)
{
	SetWindowPos(value ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void CMainFrame::OnAlwaysOnTop(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	SetAlwaysOnTop(!GetAlwaysOnTop());
}

bool CMainFrame::IsPaused() const
{
	return !m_pLocalReader;
}

void CMainFrame::Pause()
{
	SetTitle(L"Paused");
	m_pLocalReader.reset();
	m_pGlobalReader.reset();
}

void CMainFrame::Resume()
{
	SetTitle();

	if (!m_pLocalReader)
	{
		try 
		{
			m_pLocalReader = make_unique<DBWinReader>(false);
		}
		catch (std::exception&)
		{
			MessageBox(
				L"Unable to capture Win32 Messages.\n"
				L"\n"
				L"Another DebugView++ (or simular application) might be running.\n",
				m_applicationName.c_str(), MB_ICONERROR | MB_OK);
			return;
		}
	}

	if (m_tryGlobal)
	{
		try
		{
			m_pGlobalReader = make_unique<DBWinReader>(true);
		}
		catch (std::exception&)
		{
			MessageBox(
				L"Unable to capture Global Win32 Messages.\n"
				L"\n"
				L"Make sure you have appropriate permissions.\n"
				L"\n"
				L"You may need to start this application by right-clicking it and selecting\n"
				L"'Run As Administator' even if you have administrator rights.",
				m_applicationName.c_str(), MB_ICONERROR | MB_OK);
			m_tryGlobal = false;
		}
	}

	SetAutoNewLine(GetAutoNewLine());
	
	std::wstring title = L"Paused";
	if (m_pLocalReader && m_pGlobalReader)
	{
		title = L"Capture Win32 & Global Win32 Messages";
	}
	else if (m_pLocalReader)
	{
		title = L"Capture Win32";
	} 
	else if (m_pLocalReader)
	{
		title = L"Capture Global Win32";
	}
	SetTitle(title);
}

void CMainFrame::OnLogPause(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	if (IsPaused())
		Resume();
	else
		Pause();
}

void CMainFrame::OnLogGlobal(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	m_tryGlobal = !m_pGlobalReader;
	
	if (m_pLocalReader && m_tryGlobal)
		Resume();
	else
		m_pGlobalReader.reset();
}

void CMainFrame::OnViewFilter(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	int tabIdx = GetTabCtrl().GetCurSel();

	CFilterDlg dlg(GetView().GetName(), GetView().GetFilters());
	if (dlg.DoModal() != IDOK)
		return;

	GetTabCtrl().GetItem(tabIdx)->SetText(dlg.GetName().c_str());
	GetTabCtrl().UpdateLayout();
	GetTabCtrl().Invalidate();
	GetView().SetName(dlg.GetName());
	GetView().SetFilters(dlg.GetFilters());
}

void CMainFrame::OnViewFind(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	m_findDlg.SetFocus();
}

void CMainFrame::OnViewFont(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	CFontDialog dlg(&m_logfont);
	if (dlg.DoModal(*this) == IDOK)
	{
		m_logfont = dlg.m_lf;
		SetLogFont();
	}
}

void CMainFrame::SetLogFont()
{
	HFont hFont(CreateFontIndirect(&m_logfont));
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
	return GetView(std::max(0, GetTabCtrl().GetCurSel()));
}

bool CMainFrame::IsDbgViewClearMessage(const std::string& text) const
{
	return text.find("DBGVIEWCLEAR") != std::string::npos;
}

void CMainFrame::AddMessage(const Message& message)
{
	if (IsDbgViewClearMessage(message.text))
		return ClearLog();

	int index = m_logFile.Count();
	m_logFile.Add(message);
	int views = GetViewCount();
	for (int i = 0; i < views; ++i)
		GetView(i).Add(index, message);
}

} // namespace debugviewpp 
} // namespace fusion
