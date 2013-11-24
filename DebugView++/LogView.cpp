//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

#include "stdafx.h"
#include <iomanip>
#include <array>
#include <regex>
#include "dbgstream.h"
#include "Utilities.h"
#include "Resource.h"
#include "LogView.h"

namespace gj {

LogFilter::LogFilter(const std::string& text) :
	text(text), re(text)
{
}

SelectionInfo::SelectionInfo() :
	beginLine(0), endLine(0), count(0)
{
}

SelectionInfo::SelectionInfo(int beginLine, int endLine, int count) :
	beginLine(beginLine), endLine(endLine), count(count)
{
}

BEGIN_MSG_MAP_TRY(CLogView)
	MSG_WM_CREATE(OnCreate)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(NM_CUSTOMDRAW, OnCustomDraw)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_GETDISPINFO, OnGetDispInfo)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_ODSTATECHANGED, OnOdStateChanged)
	DEFAULT_REFLECTION_HANDLER()
	CHAIN_MSG_MAP(COffscreenPaint<CLogView>)
END_MSG_MAP_CATCH(ExceptionHandler)

CLogView::CLogView(CMainFrame& mainFrame, LogFile& logFile) :
	m_mainFrame(mainFrame),
	m_logFile(logFile),
	m_clockTime(false),
	m_dirty(false)
{
}

void CLogView::ExceptionHandler()
{
	MessageBox(WStr(GetExceptionMessage()), LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
}

BOOL CLogView::PreTranslateMessage(MSG* pMsg)
{
	pMsg;
	return FALSE;
}

LRESULT CLogView::OnCreate(const CREATESTRUCT* /*pCreate*/)
{
	DefWindowProc();

	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	InsertColumn(0, L"Line", LVCFMT_RIGHT, 60, 0);
	InsertColumn(1, L"Time", LVCFMT_RIGHT, 90, 0);
	InsertColumn(2, L"PID", LVCFMT_RIGHT, 60, 0);
	InsertColumn(3, L"Process", LVCFMT_LEFT, 140, 0);
	InsertColumn(4, L"Log", LVCFMT_LEFT, 600, 0);

	ApplyFilters();
	return 0;
}

LRESULT CLogView::OnCustomDraw(LPNMHDR pnmh)
{
	auto pCustomDraw = reinterpret_cast<NMLVCUSTOMDRAW*>(pnmh);

	int item = pCustomDraw->nmcd.dwItemSpec;
	switch (pCustomDraw->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
		pCustomDraw->clrTextBk = RGB(255, 255, 255);
		return CDRF_DODEFAULT;
	}

	return CDRF_DODEFAULT;
}

void CopyItemText(const std::string& s, wchar_t* buf, size_t maxLen)
{
	for (auto it = s.begin(); maxLen > 1 && it != s.end(); ++it, ++buf, --maxLen)
		*buf = *it;
	*buf = '\0';
}

void CopyItemText(const std::wstring& s, wchar_t* buf, size_t maxLen)
{
	for (auto it = s.begin(); maxLen > 1 && it != s.end(); ++it, ++buf, --maxLen)
		*buf = *it;
	*buf = '\0';
}

std::string CLogView::GetTimeText(double t) const
{
	return stringbuilder() << std::fixed << std::setprecision(6) << t;
}

std::string CLogView::GetTimeText(const SYSTEMTIME& t) const
{
	char buf[32];
	sprintf(buf, "%d:%02d:%02d.%03d", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
	return buf;
}

std::string CLogView::GetTimeText(const Message& msg) const
{
	return ""; // m_clockTime ? GetTimeText(msg.localTime) : GetTimeText(msg.time); //todo: fix delta-time
}

LRESULT CLogView::OnGetDispInfo(LPNMHDR pnmh)
{
	auto pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pnmh);
	LVITEM& item = pDispInfo->item;
	if ((item.mask & LVIF_TEXT) == 0 || item.iItem >= static_cast<int>(m_logLines.size()))
		return 0;

	int line = m_logLines[item.iItem];
	const Message& msg = m_logFile[line];
	switch (item.iSubItem)
	{
	case 0: CopyItemText(std::to_string(line + 1ULL), item.pszText, item.cchTextMax); break;
	case 1: CopyItemText(AccurateTime::GetLocalTimeString(msg.ustime), item.pszText, item.cchTextMax); break;
	case 2: CopyItemText(std::to_string(msg.processId + 0ULL), item.pszText, item.cchTextMax); break;
	case 3: CopyItemText(m_displayInfo.GetProcessName(msg.processId), item.pszText, item.cchTextMax); break;
	case 4: CopyItemText(msg.text, item.pszText, item.cchTextMax); break;
	}

	return 0;
}

SelectionInfo CLogView::GetSelectedRange() const
{
	int first = GetNextItem(-1, LVNI_SELECTED);
	if (first < 0)
		return SelectionInfo();

	int item = first;
	int last = item;
	do
	{
		last = item + 1;
		item = GetNextItem(item, LVNI_SELECTED);
	} while (item >= 0);

	return SelectionInfo(m_logLines[first], m_logLines[last], last - first);
}

LRESULT CLogView::OnOdStateChanged(LPNMHDR pnmh)
{
	auto pStateChange = reinterpret_cast<NMLVODSTATECHANGE*>(pnmh);

	m_mainFrame.SetLineRange(GetSelectedRange());

	return 0;
}

void CLogView::DoPaint(CDCHandle dc, const RECT& rcClip)
{
	dc.FillSolidRect(&rcClip, GetSysColor(COLOR_WINDOW));
 
	DefWindowProc(WM_PAINT, reinterpret_cast<WPARAM>(dc.m_hDC), 0);
}

void CLogView::Clear()
{
	SetItemCount(0);
	m_dirty = false;
	m_logLines.clear();
}

void CLogView::Add(int line, const Message& msg)
{
	if (!IsIncluded(msg.text))
		return;

	m_dirty = true;
	m_logLines.push_back(line);
}

void CLogView::BeginUpdate()
{
	int focus = GetNextItem(0, LVNI_FOCUSED);
	m_autoScrollDown = focus < 0 || focus == GetItemCount() - 1;
}

void CLogView::EndUpdate()
{
	if (m_dirty)
	{
		SetItemCount(m_logLines.size());
		if (m_autoScrollDown)
		{
			ScrollDown();
		}
		m_dirty = false;
	}
}

void CLogView::ScrollToIndex(int index, bool center)
{
	if (index < 0) return;
	if (size_t(index) >= m_logLines.size()) return;
	
	//todo: deselect any seletected items.
	
	EnsureVisible(index, false);
	SetItemState(index, LVIS_FOCUSED, LVIS_FOCUSED);

	if (center)
	{
		int maxExtraItems = GetCountPerPage() / 2;
		int maxBottomIndex = std::min<int>(m_logLines.size() - 1, index + maxExtraItems);
		EnsureVisible(maxBottomIndex, false);
	}
	//todo: make sure the listview control has focus
}

void CLogView::ScrollDown()
{
	int lastIndex = m_logLines.size()-1;
	ScrollToIndex(lastIndex, false);
}

bool CLogView::GetClockTime() const
{
	return m_clockTime;
}

void CLogView::SetClockTime(bool clockTime)
{
	m_clockTime = clockTime;
	Invalidate(false);
}

void CLogView::SelectAll()
{
	int lines = GetItemCount();
	for (int i = 0; i < lines; ++i)
		SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
}

std::string CLogView::GetItemText(int item, int subItem) const
{
	CComBSTR bstr;
	GetItemText(item, subItem, bstr.m_str);
	return std::string(bstr.m_str, bstr.m_str + bstr.Length());
}

void CLogView::Copy()
{
	std::ostringstream ss;

	int item = -1;
	while ((item = GetNextItem(item, LVNI_ALL | LVNI_SELECTED)) >= 0)
		ss <<
			GetItemText(item, 0) << "\t" <<
			GetItemText(item, 1) << "\t" <<
			GetItemText(item, 2) << "\t" <<
			GetItemText(item, 3) << "\t" <<
			GetItemText(item, 4) << "\r\n";
	const std::string& str = ss.str();

	HGLOBAL hdst = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, str.size() + 1);
	char* dst = static_cast<char*>(GlobalLock(hdst));
	std::copy(str.begin(), str.end(), stdext::checked_array_iterator<char*>(dst, str.size()));
	dst[str.size()] = '\0';
	GlobalUnlock(hdst);
	if (OpenClipboard())
	{
		EmptyClipboard();
		SetClipboardData(CF_TEXT, hdst);
		CloseClipboard();
	}
}

bool CLogView::Find(const std::string& text, int direction)
{
	int begin = std::max(GetNextItem(-1, LVNI_FOCUSED), 0);
	int line = begin + direction;
	while (line != begin)
	{
		if (line < 0)
			line += m_logLines.size();
		if (line == m_logLines.size())
			line = 0;

		if (m_logFile[m_logLines[line]].text.find(text) != std::string::npos)
		{
			EnsureVisible(line, true);
			SetItemState(line, LVIS_FOCUSED, LVIS_FOCUSED);
			SelectItem(line);
			return true;
		}
		line += direction;
	}
	return false;
}

bool CLogView::FindNext(const std::wstring& text)
{
	return Find(Str(text).str(), +1);
}

bool CLogView::FindPrevious(const std::wstring& text)
{
	return Find(Str(text).str(), -1);
}

void CLogView::LoadSettings(CRegKey& reg)
{
	std::array<wchar_t, 100> buf;
	DWORD len = buf.size();
	if (reg.QueryStringValue(L"ColWidths", buf.data(), &len) == ERROR_SUCCESS)
	{
		std::wistringstream ss(buf.data());
		int col = 0;
		int width;
		while (ss >> width)
		{
			SetColumnWidth(col, width);
			++col;
		}
	}
}

void CLogView::SaveSettings(CRegKey& reg)
{
	std::wostringstream ss;
	for (int i = 0; i < 5; ++i)
		ss << GetColumnWidth(i) << " ";
	reg.SetStringValue(L"ColWidths", ss.str().c_str());
}

std::vector<std::string> CLogView::GetFilters() const
{
	std::vector<std::string> filters;
	for (auto it = m_excludefilters.begin(); it != m_excludefilters.end(); ++it)
	{
		filters.push_back(it->text);
	}
	return filters;
}

void CLogView::SetFilters(const std::vector<std::string>& filters)
{
	std::vector<LogFilter> logFilters;
	for (auto it = filters.begin(); it != filters.end(); ++it)
		logFilters.push_back(LogFilter(*it));
	m_excludefilters.swap(logFilters);
	ApplyFilters();
}

void CLogView::ApplyFilters()
{
	m_logLines.clear();

	int count = m_logFile.Count();
	for (int i = 0; i < count; ++i)
		if (IsIncluded(m_logFile[i].text))
			m_logLines.push_back(i);
	SetItemCount(m_logLines.size());
}

bool CLogView::IsIncluded(const std::string& text) const
{
	for (auto it = m_excludefilters.begin(); it != m_excludefilters.end(); ++it)
	{
		if (std::regex_search(text, it->re))
			return false;
	}
	return true;
}

} // namespace gj
