// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <boost/utility.hpp>
#include <boost/tokenizer.hpp>
#include <psapi.h>
#include "Utilities.h"
#include "Resource.h"
#include "FilterDlg.h"
#include "AboutDlg.h"
#include "LogView.h"
#include "MainFrm.h"
#include "Win32Lib.h"

#pragma comment(lib, "psapi.lib")

namespace fusion {

const unsigned int msOnTimerPeriod = 40;	// 25 frames/second intentionally near what the human eye can still perceive

typedef boost::tokenizer<boost::char_separator<char>,
				std::string::const_iterator, std::string> NewLineTokenizer;

BEGIN_MSG_MAP_TRY(CMainFrame)
	MSG_WM_CREATE(OnCreate)
	MSG_WM_CLOSE(OnClose)
	MSG_WM_TIMER(OnTimer)
	COMMAND_ID_HANDLER_EX(ID_FILE_NEWTAB, OnFileNewTab)
	COMMAND_ID_HANDLER_EX(ID_FILE_SAVE, OnFileSave)
	COMMAND_ID_HANDLER_EX(ID_FILE_SAVE_AS, OnFileSaveAs)
	COMMAND_ID_HANDLER_EX(ID_LOG_CLEAR, OnLogClear)
	COMMAND_ID_HANDLER_EX(ID_LOG_AUTONEWLINE, OnAutoNewline)
	COMMAND_ID_HANDLER_EX(ID_LOG_PAUSE, OnLogPause)
	COMMAND_ID_HANDLER_EX(ID_LOG_GLOBAL, OnLogGlobal)
	COMMAND_ID_HANDLER_EX(ID_VIEW_CLEAR, OnViewClear)
	COMMAND_ID_HANDLER_EX(ID_VIEW_SELECTALL, OnViewSelectAll)
	COMMAND_ID_HANDLER_EX(ID_VIEW_COPY, OnViewCopy)
	COMMAND_ID_HANDLER_EX(ID_VIEW_SCROLL, OnViewScroll)
	COMMAND_ID_HANDLER_EX(ID_VIEW_TIME, OnViewTime)
	COMMAND_ID_HANDLER_EX(ID_VIEW_FIND, OnViewFind)
	COMMAND_ID_HANDLER_EX(ID_VIEW_FONT, OnViewFont)
	COMMAND_ID_HANDLER_EX(ID_VIEW_FILTER, OnViewFilter)
	COMMAND_ID_HANDLER_EX(ID_APP_ABOUT, OnAppAbout)
	NOTIFY_CODE_HANDLER_EX(NM_CLICK, OnClickTab)
	NOTIFY_CODE_HANDLER_EX(CTCN_SELCHANGE, OnChangeTab)
	NOTIFY_CODE_HANDLER_EX(CTCN_CLOSE, OnCloseTab)
	CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
	CHAIN_MSG_MAP(CTabbedFrameImpl<CMainFrame>)
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
	m_fontDlg(&GetDefaultLogFont(), CF_SCREENFONTS | CF_NOVERTFONTS | CF_SELECTSCRIPT | CF_NOSCRIPTSEL),
	m_findDlg(*this),
	m_autoNewLine(false),
	m_pLocalReader(make_unique<DBWinReader>(false))
{
#ifdef CONSOLE_DEBUG
	AllocConsole();
	freopen("CONOUT$", "wb", stdout);
#endif

	try
	{
		m_pGlobalReader = make_unique<DBWinReader>(true);
	}
	catch (std::exception e)
	{
		// todo: indicate in the UI that global messages are not available due to access rights restrictions
	}

	m_views.push_back(make_unique<CLogView>(*this, m_logFile));
}

CMainFrame::~CMainFrame()
{
}

void CMainFrame::ExceptionHandler()
{
	MessageBox(WStr(GetExceptionMessage()), LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
	UpdateUI();
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	return CTabbedFrameImpl<CMainFrame>::PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle()
{
	UIUpdateToolBar();
	UIUpdateStatusBar();
	return FALSE;
}

LRESULT CMainFrame::OnCreate(const CREATESTRUCT* /*pCreate*/)
{
	SetWindowText(WStr(LoadString(IDR_APPNAME)));

	m_findDlg.Create(*this, 0);

	CreateSimpleToolBar();
	UIAddToolBar(m_hWndToolBar);

	CreateSimpleStatusBar();
	UIAddStatusBar(m_hWndStatusBar);

	CreateTabWindow(*this, rcDefault, CTCS_CLOSEBUTTON | CTCS_DRAGREARRANGE);

	m_views.front()->Create(*this, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);
	AddTab(*m_views.front(), L"Log", 0);
	GetTabCtrl().InsertItem(1, L"+");
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

	UpdateUI();
	m_timer = SetTimer(1, msOnTimerPeriod, nullptr);
	return 0;
}

void CMainFrame::OnClose()
{
	if (m_timer)
		KillTimer(m_timer); 

	SaveSettings();
	DestroyWindow();

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
	UISetCheck(ID_LOG_AUTONEWLINE, m_autoNewLine);
	UISetCheck(ID_LOG_PAUSE, !m_pLocalReader);
	UISetCheck(ID_LOG_GLOBAL, m_pGlobalReader);
}

void CMainFrame::UpdateStatusBar()
{
	//UISetText(0, L"Ready");
}

void CMainFrame::ProcessLines(const LinesList& lines)
{
#ifdef CONSOLE_DEBUG
	if (lines.size() > 0)
		printf("incoming lines: %d\n", lines.size());
#endif

	if (m_logFile.Empty() && !lines.empty())
		m_timeOffset = lines[0].time;

	for (auto it = m_views.begin(); it != m_views.end(); ++it)
		(*it)->BeginUpdate();

	for (auto it = lines.begin(); it != lines.end(); ++it)
		AddMessage(Message(it->time - m_timeOffset, it->systemTime, it->pid, it->message));

	for (auto it = m_views.begin(); it != m_views.end(); ++it)
		(*it)->EndUpdate();

}

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
	if (m_pLocalReader)
	{
		ProcessLines(m_pLocalReader->GetLines());
	}
	if (m_pGlobalReader)
	{
		ProcessLines(m_pGlobalReader->GetLines());
	}
}

const wchar_t* RegistryPath = L"Software\\DjeeDjay\\DebugView++";

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
	DWORD options;
	if (reg.QueryDWORDValue(L"ClockTime", options) == ERROR_SUCCESS)
		m_views[0]->SetClockTime(options != 0);

	for (size_t i = 0; ; ++i)
	{
		CRegKey regView;
		if (regView.Open(reg, WStr(wstringbuilder() << L"Views\\View" << i)) != ERROR_SUCCESS)
			break;

		if (i > 0)
			AddFilterView(RegGetStringValue(regView));
		GetView().LoadSettings(regView);
	}

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
	reg.RecurseDeleteKey(L"Views");
	for (size_t i = 0; i < m_views.size(); ++i)
	{
		CRegKey regView;
		regView.Create(reg, WStr(wstringbuilder() << L"Views\\View" << i));
		regView.SetValue(GetTabCtrl().GetItem(i)->GetTextRef());
		m_views[i]->SaveSettings(regView);
	}
	reg.SetDWORDValue(L"ClockTime", m_views[0]->GetClockTime());
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

	std::wstring unit = L"s";
	if (seconds < 1)
	{
		seconds *= 1e3;
		unit = L"ms";
	}
	if (seconds < 1)
	{
		seconds *= 1e3;
		unit = L"µs";
	}
	if (seconds < 1)
	{
		seconds *= 1e3;
		unit = L"ns";
	}

	return wstringbuilder() << std::setprecision(6) << seconds << L" " << unit;
}

void CMainFrame::SetLineRange(const SelectionInfo& selection)
{
	if (selection.count > 0)
	{
		double dt = m_logFile[selection.endLine - 1].time - m_logFile[selection.beginLine].time;
		std::wstring text = wstringbuilder() << FormatDuration(dt) << L" (" << selection.count << " messages)";
		UISetText(ID_DEFAULT_PANE, text.c_str());
	}
	else
	{
		UISetText(ID_DEFAULT_PANE, L"Ready");
	}
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

void CMainFrame::AddFilterView(const std::wstring& name, std::vector<LogFilter> filters)
{
	m_views.push_back(make_unique<CLogView>(*this, m_logFile, filters));
	m_views.back()->Create(*this, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);

	int newIndex = GetTabCtrl().GetItemCount() - 1;
	GetTabCtrl().InsertItem(newIndex, name.c_str());
	GetTabCtrl().GetItem(newIndex)->SetTabView(*m_views.back());
	GetTabCtrl().SetCurSel(newIndex);
	ShowTabControl();
}

LRESULT CMainFrame::OnClickTab(NMHDR* pnmh)
{
	auto& nmhdr = *reinterpret_cast<NMCTCITEM*>(pnmh);
	if (nmhdr.hdr.hwndFrom != GetTabCtrl())
		return FALSE;

	int plusIndex = GetTabCtrl().GetItemCount() - 1;
	if (nmhdr.iItem == plusIndex)
	{
		AddFilterView();
		return TRUE;
	}
	return FALSE;
}

LRESULT CMainFrame::OnChangeTab(NMHDR* pnmh)
{
	SetLineRange(GetView().GetSelectedRange());
	SetMsgHandled(FALSE);
	return 0;
}

LRESULT CMainFrame::OnCloseTab(NMHDR* pnmh)
{
	auto& nmhdr = *reinterpret_cast<NMCTCITEM*>(pnmh);
	int filterIndex = nmhdr.iItem;
	if (filterIndex > 0 && filterIndex < static_cast<int>(m_views.size()))
	{
		GetTabCtrl().DeleteItem(filterIndex);
		auto it = m_views.begin() + filterIndex;
		(*it)->DestroyWindow();
		m_views.erase(it);
		if (filterIndex == m_views.size())
			GetTabCtrl().SetCurSel(filterIndex - 1);
		if (m_views.size() == 1)
			HideTabControl();
	}
	return 0;
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

void CMainFrame::OnViewClear(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	GetView().Clear();
}

void CMainFrame::OnViewSelectAll(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	GetView().SelectAll();
}

void CMainFrame::OnLogClear(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	for (auto it = m_views.begin(); it != m_views.end(); ++it)
		(*it)->Clear();
	m_logFile.Clear();
}

void CMainFrame::OnViewScroll(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	GetView().SetScroll(!GetView().GetScroll());
	UpdateUI();
}

void CMainFrame::OnViewTime(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	GetView().SetClockTime(!GetView().GetClockTime());
	UpdateUI();
}

void CMainFrame::OnAutoNewline(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	m_autoNewLine = !m_autoNewLine;
	UpdateUI();
}

void CMainFrame::OnLogPause(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	if (m_pLocalReader)
		m_pLocalReader.reset();
	else
		m_pLocalReader = make_unique<DBWinReader>(false);

	UpdateUI();
}

void CMainFrame::OnLogGlobal(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	if (m_pGlobalReader)
		m_pGlobalReader.reset();
	else
		m_pGlobalReader = make_unique<DBWinReader>(true);
	UpdateUI();
}

void CMainFrame::OnViewFilter(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	int tabIdx = GetTabCtrl().GetCurSel();

	CFilterDlg dlg(GetTabCtrl().GetItem(tabIdx)->GetTextRef(), GetView().GetFilters());
	if (dlg.DoModal() != IDOK)
		return;

	GetTabCtrl().GetItem(tabIdx)->SetText(dlg.GetName().c_str());
	GetView().SetFilters(dlg.GetFilters());
}

void CMainFrame::OnViewCopy(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	GetView().Copy();
}

void CMainFrame::OnViewFind(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	m_findDlg.ShowWindow(SW_SHOW);
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
	HFONT hFont = CreateFontIndirect(&lf);
	if (hFont)
		GetView().SetFont(hFont);
}

void CMainFrame::OnAppAbout(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
}

CLogView& CMainFrame::GetView()
{
	assert(!m_views.empty());

	int i = GetTabCtrl().GetCurSel();
	return i >= 0 && i < static_cast<int>(m_views.size()) ? *m_views[i] : *m_views[0];
}


void CMainFrame::AddMessage(const Message& message)
{
	Message msg(message);

	std::string text;
	boost::char_separator<char> seporators("\r\n");
	NewLineTokenizer tok(msg.text, seporators);
	for (auto it = tok.begin(); it != tok.end(); ++it)
	{
		if (m_autoNewLine)
		{
			msg.text = *it;
			AddPreppedMessage(msg);
		}
		else
		{
			text += *it;
		}
	}

	if (!m_autoNewLine)
	{
		msg.text = text;
		AddPreppedMessage(msg);
	}
}

void CMainFrame::AddPreppedMessage(const Message& msg)
{
	int index = m_logFile.Count();

	m_logFile.Add(msg);
	for (auto it = m_views.begin(); it != m_views.end(); ++it)
		(*it)->Add(index, msg);
}


} // namespace fusion
