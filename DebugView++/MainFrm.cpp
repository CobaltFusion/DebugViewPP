//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://boosttestui.wordpress.com/ for the boosttestui home page.

#include "stdafx.h"
#include <boost/utility.hpp>
#include <psapi.h>
#include "Utilities.h"
#include "Resource.h"
#include "FilterDlg.h"
#include "AboutDlg.h"
#include "LogView.h"
#include "MainFrm.h"
#include "Win32Lib.h"

#pragma comment(lib, "psapi.lib")

namespace gj {

const unsigned int msOnTimerPeriod = 40;	// 25 frames/second intentionally near what the human eye can still perceive

BEGIN_MSG_MAP_TRY(CMainFrame)
	MSG_WM_CREATE(OnCreate)
	MSG_WM_CLOSE(OnClose)
	MSG_WM_TIMER(OnTimer)
	COMMAND_ID_HANDLER_EX(ID_FILE_SAVE, OnFileSave)
	COMMAND_ID_HANDLER_EX(ID_LOG_SELECTALL, OnLogSelectAll)
	COMMAND_ID_HANDLER_EX(ID_LOG_CLEAR, OnLogClear)
	COMMAND_ID_HANDLER_EX(ID_LOG_TIME, OnLogTime)
	COMMAND_ID_HANDLER_EX(ID_LOG_FILTER, OnLogFilter)
	COMMAND_ID_HANDLER_EX(ID_LOG_COPY, OnLogCopy)
	COMMAND_ID_HANDLER_EX(ID_LOG_PAUSE, OnLogPause)
	COMMAND_ID_HANDLER_EX(ID_LOG_FIND, OnLogFind)
	COMMAND_ID_HANDLER_EX(ID_VIEW_FONT, OnViewFont)
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
	m_filterNr(0),
	m_fontDlg(&GetDefaultLogFont(), CF_SCREENFONTS | CF_NOVERTFONTS | CF_SELECTSCRIPT | CF_NOSCRIPTSEL),
	m_findDlg(*this),
	m_paused(false),
	m_localReader(false)
//	m_globalReader(true),
{

#define CONSOLE_DEBUG
#ifdef CONSOLE_DEBUG
	AllocConsole();
	freopen("CONOUT$", "wb", stdout);
#endif

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
	LoadSettings();

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != nullptr);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	UpdateUI();
	m_timer = SetTimer(1, msOnTimerPeriod, NULL);
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
	UISetText(0, L"Ready");
	UISetCheck(ID_LOG_TIME, GetView().GetClockTime());
	UISetCheck(ID_LOG_PAUSE, m_paused);
}

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
	auto lines = m_localReader.GetLines();

#ifdef CONSOLE_DEBUG
	if (lines.size() > 0)
		printf("incoming lines: %d\n", lines.size());
#endif

	for (auto it = m_views.begin(); it != m_views.end(); ++it)
		(*it)->BeginUpdate();

	for (auto it = lines.begin(); it != lines.end(); ++it)
		AddMessage(Message(m_localReader.GetQPCOffsetInUs(it->qpctime), AccurateTime::GetSystemTimeInUs(it->systemtime), it->pid, it->message));

	for (auto it = m_views.begin(); it != m_views.end(); ++it)
		(*it)->EndUpdate();
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

	m_views[0]->LoadSettings(reg);

	DWORD options;
	if (reg.QueryDWORDValue(L"ClockTime", options) == ERROR_SUCCESS)
		m_views[0]->SetClockTime(options != 0);

	return true;
}

void CMainFrame::SaveSettings()
{
	WINDOWPLACEMENT placement;
	placement.length = sizeof(placement);
	GetWindowPlacement(&placement);

	CRegKey reg;
	reg.Create(HKEY_CURRENT_USER, RegistryPath);
	reg.SetDWORDValue(L"x", placement.rcNormalPosition.left);
	reg.SetDWORDValue(L"y", placement.rcNormalPosition.top);
	reg.SetDWORDValue(L"width", placement.rcNormalPosition.right - placement.rcNormalPosition.left);
	reg.SetDWORDValue(L"height", placement.rcNormalPosition.bottom - placement.rcNormalPosition.top);
	m_views[0]->SaveSettings(reg);
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
		unit = L"us";
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
		double dt = AccurateTime::GetDeltaFromUs(m_logFile[selection.beginLine].qpctime, m_logFile[selection.endLine - 1].qpctime);
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
	std::wstring name = wstringbuilder() << L"Filter " << m_filterNr;

	CFilterDlg dlg(name);
	if (dlg.DoModal() != IDOK)
		return;

	m_views.push_back(make_unique<CLogView>(*this, m_logFile));
	m_views.back()->Create(*this, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);
	m_views.back()->SetFilters(dlg.GetFilters());

	int newIndex = GetTabCtrl().GetItemCount() - 1;
	GetTabCtrl().InsertItem(newIndex, dlg.GetName().c_str());
	GetTabCtrl().GetItem(newIndex)->SetTabView(*m_views.back());
	GetTabCtrl().SetCurSel(newIndex);
	ShowTabControl();
}

LRESULT CMainFrame::OnClickTab(NMHDR* pnmh)
{
	NMCTCITEM* pNmCtcItem = reinterpret_cast<NMCTCITEM*>(pnmh);
	if (pNmCtcItem->hdr.hwndFrom != GetTabCtrl())
		return FALSE;

	int plusIndex = GetTabCtrl().GetItemCount() - 1;
	if (pNmCtcItem->iItem == plusIndex)
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
	auto pNmCtcItem = reinterpret_cast<NMCTCITEM*>(pnmh);
	int filterIndex = pNmCtcItem->iItem;
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

void CMainFrame::OnFileSave(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
}

void CMainFrame::OnLogSelectAll(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	GetView().SelectAll();
}

void CMainFrame::OnLogClear(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	for (auto it = m_views.begin(); it != m_views.end(); ++it)
		(*it)->Clear();
	m_logFile.Clear();	// todo: deal with multiple views, but for now release all memory to verify implemention
}

void CMainFrame::OnLogTime(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	GetView().SetClockTime(!GetView().GetClockTime());
	UpdateUI();
}

void CMainFrame::OnLogFilter(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	int tabIdx = GetTabCtrl().GetCurSel();
	if (tabIdx == 0)
		return AddFilterView();

	CFilterDlg dlg(GetTabCtrl().GetItem(tabIdx)->GetTextRef(), GetView().GetFilters());
	if (dlg.DoModal() != IDOK)
		return;

	GetTabCtrl().GetItem(tabIdx)->SetText(dlg.GetName().c_str());
	GetView().SetFilters(dlg.GetFilters());
}

void CMainFrame::OnLogCopy(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	GetView().Copy();
}

void CMainFrame::OnLogPause(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	m_paused = !m_paused;
	UpdateUI();
}

void CMainFrame::OnLogFind(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
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

void CMainFrame::AddMessage(const Message& msg)
{
	if (m_paused)
		return;

	int index = m_logFile.Count();

	m_logFile.Add(msg);
	for (auto it = m_views.begin(); it != m_views.end(); ++it)
		(*it)->Add(index, msg);
}

} // namespace gj
