// (C) Copyright Gert-Jan de Vos 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

#include "stdafx.h"
#include <iomanip>
#include <array>
#include <regex>
#include "Win32Lib.h"
#include "dbgstream.h"
#include "Utilities.h"
#include "Resource.h"
#include "MainFrm.h"
#include "LogView.h"

namespace gj {

SelectionInfo::SelectionInfo() :
	beginLine(0), endLine(0), count(0)
{
}

SelectionInfo::SelectionInfo(int beginLine, int endLine, int count) :
	beginLine(beginLine), endLine(endLine), count(count)
{
}

LogLine::LogLine(int line, TextColor color) :
	line(line), color(color)
{
}

BEGIN_MSG_MAP_TRY(CLogView)
	MSG_WM_CREATE(OnCreate)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(NM_CLICK, OnClick)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_ITEMCHANGED, OnItemChanged)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(NM_CUSTOMDRAW, OnCustomDraw)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_GETDISPINFO, OnGetDispInfo)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_ODSTATECHANGED, OnOdStateChanged)
	DEFAULT_REFLECTION_HANDLER()
	CHAIN_MSG_MAP(COffscreenPaint<CLogView>)
END_MSG_MAP_CATCH(ExceptionHandler)

CLogView::CLogView(CMainFrame& mainFrame, LogFile& logFile, std::vector<LogFilter> filters) :
	m_mainFrame(mainFrame),
	m_logFile(logFile),
	m_filters(std::move(filters)),
	m_clockTime(false),
	m_autoScrollDown(true),
	m_dirty(false),
	m_insidePaint(false)
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

LRESULT CLogView::OnClick(NMHDR* pnmh)
{
	return 0;
}

LRESULT CLogView::OnItemChanged(NMHDR* pnmh)
{
	auto pListView = reinterpret_cast<NMLISTVIEW*>(pnmh);

	if ((pListView->uNewState & LVIS_FOCUSED) == 0 ||
		pListView->iItem < 0  ||
		static_cast<size_t>(pListView->iItem) >= m_logLines.size())
		return 0;

	m_autoScrollDown = pListView->iItem == GetItemCount() - 1;
	m_mainFrame.UpdateUI();
	return 0;
}

RECT CLogView::GetItemRect(int iItem, unsigned code) const
{
	RECT rect;
	CListViewCtrl::GetItemRect(iItem, &rect, code);
	return rect;
}

RECT CLogView::GetSubItemRect(int iItem, int iSubItem, unsigned code) const
{
	int subitemCount = GetHeader().GetItemCount();

	RECT rect;
	CListViewCtrl::GetSubItemRect(iItem, iSubItem, code, &rect);
	if (iSubItem == 0 && subitemCount > 1)
		rect.right = GetSubItemRect(iItem, 1, code).left;
	return rect;
}

unsigned GetTextAlign(const HDITEM& item)
{
	switch (item.fmt & HDF_JUSTIFYMASK)
	{
	case HDF_LEFT: return DT_LEFT;
	case HDF_CENTER: return DT_CENTER;
	case HDF_RIGHT: return DT_RIGHT;
	}
	return HDF_LEFT;
}

class ScopedBkColor
{
public:
	ScopedBkColor(HDC hdc, COLORREF col) :
		m_hdc(hdc),
		m_col(SetBkColor(hdc, col))
	{
	}

	~ScopedBkColor()
	{
		SetBkColor(m_hdc, m_col);
	}

private:
	HDC m_hdc;
	COLORREF m_col;
};

class ScopedTextColor
{
public:
	ScopedTextColor(HDC hdc, COLORREF col) :
		m_hdc(hdc),
		m_col(SetTextColor(hdc, col))
	{
	}

	~ScopedTextColor()
	{
		SetTextColor(m_hdc, m_col);
	}

private:
	HDC m_hdc;
	COLORREF m_col;
};

void CLogView::DrawSubItem(CDCHandle dc, int iItem, int iSubItem) const
{
	auto text = GetItemWText(iItem, iSubItem);
//	dc.GetTextExtent(LPCTSTR lpszString, int nCount, LPSIZE lpSize);
	RECT rect = GetSubItemRect(iItem, iSubItem, LVIR_BOUNDS);
	int margin = GetHeader().GetBitmapMargin();
	rect.left += margin;
	rect.right -= margin;

	HDITEM item;
	item.mask = HDI_FORMAT;
	unsigned align = (GetHeader().GetItem(iSubItem, &item)) ? GetTextAlign(item) : HDF_LEFT;
	dc.DrawText(text.c_str(), text.size(), &rect, align | DT_NOCLIP | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

void CLogView::DrawItem(CDCHandle dc, int iItem, unsigned iItemState) const
{
	auto rect = GetItemRect(iItem, LVIR_BOUNDS);

	bool selected = GetItemState(iItem, LVIS_SELECTED) == LVIS_SELECTED;
	bool focused = GetItemState(iItem, LVIS_FOCUSED) == LVIS_FOCUSED;
	auto bkColor = selected ? GetSysColor(COLOR_HIGHLIGHT) : m_logLines[iItem].color.back;
	auto txColor = selected ? GetSysColor(COLOR_HIGHLIGHTTEXT) : m_logLines[iItem].color.fore;
	dc.FillSolidRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, bkColor);
	ScopedBkColor bcol(dc, bkColor);
	ScopedTextColor tcol(dc, txColor);

	int subitemCount = GetHeader().GetItemCount();
	for (int i = 0; i < subitemCount; ++i)
		DrawSubItem(dc, iItem, i);
	if (focused)
		dc.DrawFocusRect(&rect);
}

LRESULT CLogView::OnCustomDraw(NMHDR* pnmh)
{
	// See: http://stackoverflow.com/questions/938896/flickering-in-listview-with-ownerdraw-and-virtualmode
	if (!m_insidePaint)
		return CDRF_SKIPDEFAULT;

	auto pCustomDraw = reinterpret_cast<NMLVCUSTOMDRAW*>(pnmh);

	switch (pCustomDraw->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		DrawItem(pCustomDraw->nmcd.hdc, pCustomDraw->nmcd.dwItemSpec, pCustomDraw->nmcd.uItemState);
		return CDRF_SKIPDEFAULT;
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

std::string GetTimeText(double time)
{
	return stringbuilder() << std::fixed << std::setprecision(6) << time;
}

std::string GetTimeText(const SYSTEMTIME& st)
{
	char buf[32];
	sprintf(buf, "%d:%02d:%02d.%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	return buf;
}

std::string GetTimeText(const FILETIME& ft)
{
	return GetTimeText(FileTimeToSystemTime(FileTimeToLocalFileTime(ft)));
}

LRESULT CLogView::OnGetDispInfo(NMHDR* pnmh)
{
	auto pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pnmh);
	LVITEM& item = pDispInfo->item;
	if ((item.mask & LVIF_TEXT) == 0 || item.iItem >= static_cast<int>(m_logLines.size()))
		return 0;

	int line = m_logLines[item.iItem].line;
	const Message& msg = m_logFile[line];
	std::string timeString = m_clockTime ? GetTimeText(msg.systemTime) : GetTimeText(msg.time);

	switch (item.iSubItem)
	{
	case 0: CopyItemText(std::to_string(line + 1ULL), item.pszText, item.cchTextMax); break;
	case 1: CopyItemText(timeString, item.pszText, item.cchTextMax); break;
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

	return SelectionInfo(m_logLines[first].line, m_logLines[last].line, last - first);
}

LRESULT CLogView::OnOdStateChanged(NMHDR* pnmh)
{
	auto pStateChange = reinterpret_cast<NMLVODSTATECHANGE*>(pnmh);

	m_mainFrame.SetLineRange(GetSelectedRange());

	return 0;
}

void CLogView::DoPaint(CDCHandle dc, const RECT& rcClip)
{
	m_insidePaint = true;

	dc.FillSolidRect(&rcClip, GetSysColor(COLOR_WINDOW));
 
	DefWindowProc(WM_PAINT, reinterpret_cast<WPARAM>(dc.m_hDC), 0);
	m_insidePaint = false;
}

bool CLogView::GetScroll() const
{
	return m_autoScrollDown;
}

void CLogView::SetScroll(bool enable)
{
	m_autoScrollDown = enable;
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
	m_logLines.push_back(LogLine(line, GetTextColor(msg.text)));
}

void CLogView::BeginUpdate()
{
}

void CLogView::EndUpdate()
{
	if (m_dirty)
	{
		SetItemCountEx(m_logLines.size(), LVSICF_NOSCROLL);
		if (m_autoScrollDown)
			ScrollDown();

		m_dirty = false;
	}
}

void CLogView::ScrollToIndex(int index, bool center)
{
	if (index < 0 || index >= static_cast<int>(m_logLines.size()))
		return;
	
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
	ScrollToIndex(m_logLines.size() - 1, false);
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

std::wstring CLogView::GetItemWText(int item, int subItem) const
{
	CComBSTR bstr;
	GetItemText(item, subItem, bstr.m_str);
	return std::wstring(bstr.m_str, bstr.m_str + bstr.Length());
}

std::string CLogView::GetItemText(int item) const
{
	return stringbuilder() <<
		GetItemText(item, 0) << "\t" <<
		GetItemText(item, 1) << "\t" <<
		GetItemText(item, 2) << "\t" <<
		GetItemText(item, 3) << "\t" <<
		GetItemText(item, 4);
}

void CLogView::Copy()
{
	std::ostringstream ss;

	int item = -1;
	while ((item = GetNextItem(item, LVNI_ALL | LVNI_SELECTED)) >= 0)
		ss << GetItemText(item) << "\n";
	const std::string& str = ss.str();

	HGlobal hdst(GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, str.size() + 1));
	GlobalLock<char> lock(hdst);
	std::copy(str.begin(), str.end(), stdext::checked_array_iterator<char*>(lock.Ptr(), str.size()));
	lock.Ptr()[str.size()] = '\0';
	if (OpenClipboard())
	{
		EmptyClipboard();
		SetClipboardData(CF_TEXT, hdst.release());
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

		if (m_logFile[m_logLines[line].line].text.find(text) != std::string::npos)
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

int FilterTypeToInt(FilterType::type value)
{
	return value;
}

FilterType::type IntToFilterType(int value)
{
	switch (value)
	{
	case FilterType::Include: return FilterType::Include;
	case FilterType::Exclude: return FilterType::Exclude;
	case FilterType::Highlight: return FilterType::Highlight;
	default: assert(!"Unexpected FilterType"); break;
	}
	throw std::invalid_argument("bad FilterType!");
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

	for (size_t i = 0; ; ++i)
	{
		CRegKey regFilter;
		if (regFilter.Open(reg, WStr(wstringbuilder() << L"Filters\\Filter" << i)) != ERROR_SUCCESS)
			break;

		m_filters.push_back(LogFilter(
			Str(RegGetStringValue(regFilter)),
			IntToFilterType(RegGetDWORDValue(regFilter, L"Type")),
			RegGetDWORDValue(regFilter, L"BgColor"),
			RegGetDWORDValue(regFilter, L"FgColor"),
			RegGetDWORDValue(regFilter, L"Enable") != 0));
	}
	ApplyFilters();
}

void CLogView::SaveSettings(CRegKey& reg)
{
	std::wostringstream ss;
	for (int i = 0; i < 5; ++i)
		ss << GetColumnWidth(i) << " ";
	reg.SetStringValue(L"ColWidths", ss.str().c_str());;

	for (size_t i = 0; i < m_filters.size(); ++i)
	{
		CRegKey regFilter;
		regFilter.Create(reg, WStr(wstringbuilder() << L"Filters\\Filter" << i));
		regFilter.SetValue(WStr(m_filters[i].text.c_str()));
		regFilter.SetDWORDValue(L"Type", FilterTypeToInt(m_filters[i].type));
		regFilter.SetDWORDValue(L"BgColor", m_filters[i].bgColor);
		regFilter.SetDWORDValue(L"FgColor", m_filters[i].fgColor);
		regFilter.SetDWORDValue(L"Enable", m_filters[i].enable);
	}
}

void CLogView::Save(const std::wstring& fileName) const
{
	std::ofstream file(fileName);

	int lines = GetItemCount();
	for (int i = 0; i < lines; ++i)
		file << GetItemText(i) << "\n";

	file.close();
	if (!file)
		ThrowLastError(fileName);
}

std::vector<LogFilter> CLogView::GetFilters() const
{
	return m_filters;
}

void CLogView::SetFilters(std::vector<LogFilter> logFilters)
{
	m_filters.swap(logFilters);
	ApplyFilters();
}

void CLogView::ApplyFilters()
{
	m_logLines.clear();

	int count = m_logFile.Count();
	for (int i = 0; i < count; ++i)
	{
		if (IsIncluded(m_logFile[i].text))
			m_logLines.push_back(LogLine(i, GetTextColor(m_logFile[i].text)));
	}
	SetItemCount(m_logLines.size());
}

TextColor CLogView::GetTextColor(const std::string& text) const
{
	for (auto it = m_filters.begin(); it != m_filters.end(); ++it)
	{
		if (it->enable && it->type == FilterType::Highlight && std::regex_search(text, it->re))
			return TextColor(it->bgColor, it->fgColor);
	}

	return TextColor(GetSysColor(COLOR_WINDOW), GetSysColor(COLOR_WINDOWTEXT));
}

bool CLogView::IsIncluded(const std::string& text) const
{
	for (auto it = m_filters.begin(); it != m_filters.end(); ++it)
	{
		if (!it->enable)
			continue;

		switch (it->type)
		{
		case FilterType::Include:
		case FilterType::Exclude:
			if (std::regex_search(text, it->re))
				return it->type == FilterType::Include;
			break;

		default:
			break;
		}
	}
	return true;
}

} // namespace gj
