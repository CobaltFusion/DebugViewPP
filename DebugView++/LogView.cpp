// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <iomanip>
#include <array>
#include <regex>
#include <boost/algorithm/string.hpp>
#include "Win32Lib.h"
#include "Utilities.h"
#include "Resource.h"
#include "MainFrame.h"
#include "LogView.h"

namespace fusion {

SelectionInfo::SelectionInfo() :
	beginLine(0), endLine(0), count(0)
{
}

SelectionInfo::SelectionInfo(int beginLine, int endLine, int count) :
	beginLine(beginLine), endLine(endLine), count(count)
{
}

LogLine::LogLine(int line, TextColor color) :
	bookmark(false), line(line), color(color)
{
}

BEGIN_MSG_MAP_TRY(CLogView)
	MSG_WM_CREATE(OnCreate)
	MSG_WM_CONTEXTMENU(OnContextMenu)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(NM_CLICK, OnClick)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(NM_DBLCLK, OnDblClick)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_ITEMCHANGED, OnItemChanged)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(NM_CUSTOMDRAW, OnCustomDraw)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_GETDISPINFO, OnGetDispInfo)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_ODSTATECHANGED, OnOdStateChanged)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_INCREMENTALSEARCH, OnIncrementalSearch)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_ODCACHEHINT, OnOdCacheHint)
	COMMAND_ID_HANDLER_EX(ID_VIEW_CLEAR, OnViewClear)
	COMMAND_ID_HANDLER_EX(ID_VIEW_SELECTALL, OnViewSelectAll)
	COMMAND_ID_HANDLER_EX(ID_VIEW_COPY, OnViewCopy)
	COMMAND_ID_HANDLER_EX(ID_VIEW_SCROLL, OnViewScroll)
	COMMAND_ID_HANDLER_EX(ID_VIEW_TIME, OnViewTime)
	COMMAND_ID_HANDLER_EX(ID_VIEW_HIDE_HIGHLIGHT, OnViewHideHighlight)
	COMMAND_ID_HANDLER_EX(ID_VIEW_FIND_NEXT, OnViewFindNext)
	COMMAND_ID_HANDLER_EX(ID_VIEW_FIND_PREVIOUS, OnViewFindPrevious)
	COMMAND_ID_HANDLER_EX(ID_VIEW_NEXT_PROCESS, OnViewNextProcess)
	COMMAND_ID_HANDLER_EX(ID_VIEW_PREVIOUS_PROCESS, OnViewPreviousProcess)
	COMMAND_ID_HANDLER_EX(ID_VIEW_EXCLUDE_PROCESS, OnViewExclude)
	COMMAND_ID_HANDLER_EX(ID_VIEW_BOOKMARK, OnViewBookmark)
	COMMAND_ID_HANDLER_EX(ID_VIEW_NEXT_BOOKMARK, OnViewNextBookmark)
	COMMAND_ID_HANDLER_EX(ID_VIEW_PREVIOUS_BOOKMARK, OnViewPreviousBookmark)
	DEFAULT_REFLECTION_HANDLER()
	CHAIN_MSG_MAP(COffscreenPaint<CLogView>)
END_MSG_MAP_CATCH(ExceptionHandler)

CLogView::CLogView(CMainFrame& mainFrame, LogFile& logFile, LogFilter filter) :
	m_mainFrame(mainFrame),
	m_logFile(logFile),
	m_filter(std::move(filter)),
	m_clockTime(false),
	m_autoScrollDown(true),
	m_dirty(false),
	m_insidePaint(false),
	m_hBookmarkIcon(static_cast<HICON>(LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_BOOKMARK), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR)))
{
}

void CLogView::ExceptionHandler()
{
	MessageBox(WStr(GetExceptionMessage()), LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
}

LRESULT CLogView::OnCreate(const CREATESTRUCT* /*pCreate*/)
{
	DefWindowProc();

	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	InsertColumn(0, L"", LVCFMT_RIGHT, 20, 0);
	InsertColumn(1, L"Line", LVCFMT_RIGHT, 60, 0);
	InsertColumn(2, L"Time", LVCFMT_RIGHT, 90, 0);
	InsertColumn(3, L"PID", LVCFMT_RIGHT, 60, 0);
	InsertColumn(4, L"Process", LVCFMT_LEFT, 140, 0);
	InsertColumn(5, L"Log", LVCFMT_LEFT, 600, 0);

	ApplyFilters();
	return 0;
}

void CLogView::OnContextMenu(HWND /*hWnd*/, CPoint pt)
{
	if (pt == CPoint(-1, -1))
	{
		RECT rect = GetItemRect(GetNextItem(-1, LVNI_ALL | LVNI_FOCUSED), LVIR_LABEL);
		pt = CPoint(rect.left, rect.bottom - 1);
	}
	else
	{
		ScreenToClient(&pt);
	}

	LVHITTESTINFO info;
	info.flags = 0;
	info.pt = pt;
	SubItemHitTest(&info);
	if ((info.flags & LVHT_ONITEM) == 0)
		return;

	CMenu menuContext;
	menuContext.LoadMenu(info.iSubItem == 4 ? IDR_PROCESS_CONTEXTMENU : IDR_VIEW_CONTEXTMENU);
	CMenuHandle menuPopup(menuContext.GetSubMenu(0));
	ClientToScreen(&pt);
	menuPopup.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_mainFrame);
}

int GetTextOffset(HDC hdc, const std::string& s, int xPos)
{
	int nFit;
	SIZE size;
	if (!GetTextExtentExPointA(hdc, s.c_str(), s.size(), xPos, &nFit, nullptr, &size) || nFit < 0 || nFit >= static_cast<int>(s.size()))
		return -1;
	return nFit;
}

int GetTextOffset(HDC hdc, const std::wstring& s, int xPos)
{
	int nFit;
	SIZE size;
	if (!GetTextExtentExPointW(hdc, s.c_str(), s.size(), xPos, &nFit, nullptr, &size) || nFit < 0 || nFit >= static_cast<int>(s.size()))
		return -1;
	return nFit;
}

bool iswordchar(int c)
{
	return isalnum(c) || c == '_';
}

LRESULT CLogView::OnClick(NMHDR* pnmh)
{
	auto& nmhdr = *reinterpret_cast<NMITEMACTIVATE*>(pnmh);

	LVHITTESTINFO info;
	info.flags = 0;
	info.pt = nmhdr.ptAction;
	SubItemHitTest(&info);
	if ((info.flags & LVHT_ONITEM) != 0 && info.iSubItem == 0)
		ToggleBookmark(info.iItem);
	return 0;
}

LRESULT CLogView::OnDblClick(NMHDR* pnmh)
{
	auto& nmhdr = *reinterpret_cast<NMITEMACTIVATE*>(pnmh);

	if (nmhdr.iItem < 0 || static_cast<size_t>(nmhdr.iItem) >= m_logLines.size())
		return 0;

	auto msg = m_logFile[m_logLines[nmhdr.iItem].line];

	auto rect = GetSubItemRect(0, 5, LVIR_BOUNDS);
	CDCHandle dc(GetDC());
	GdiObjectSelection font(dc, GetFont());
	int nFit = GetTextOffset(dc, msg.text, nmhdr.ptAction.x - rect.left);
	if (nFit < 0)
		return 0;

	int begin = nFit;
	while (begin > 0)
	{
		if (!iswordchar(msg.text[begin - 1]))
			break;
		--begin;
	}
	int end = nFit;
	while (end < static_cast<int>(msg.text.size()))
	{
		if (!iswordchar(msg.text[end]))
			break;
		++end;
	}
	SetHighlightText(std::wstring(msg.text.begin() + begin, msg.text.begin() + end));
	return 0;
}

LRESULT CLogView::OnItemChanged(NMHDR* pnmh)
{
	auto& nmhdr = *reinterpret_cast<NMLISTVIEW*>(pnmh);

	if ((nmhdr.uNewState & LVIS_FOCUSED) == 0 ||
		nmhdr.iItem < 0  ||
		static_cast<size_t>(nmhdr.iItem) >= m_logLines.size())
		return 0;

	m_autoScrollDown = nmhdr.iItem == GetItemCount() - 1;
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

SIZE GetTextSize(CDCHandle dc, const std::wstring& text, int length)
{
	SIZE size;
	dc.GetTextExtent(text.c_str(), length, &size);
	return size;
}

void ExtTextOut(HDC hdc, const POINT& pt, const RECT& rect, const std::wstring& text)
{
	::ExtTextOutW(hdc, pt.x, pt.y, ETO_CLIPPED | ETO_OPAQUE, &rect, text.c_str(), text.size(), nullptr);
}

void AddEllipsis(HDC hdc, std::wstring& text, int width)
{
	static const std::wstring ellipsis(L"...");
	int pos = GetTextOffset(hdc, text, width - GetTextSize(hdc, ellipsis, ellipsis.size()).cx);
	if (pos >= 0 && pos < static_cast<int>(text.size()))
		text = text.substr(0, pos) + ellipsis;
}

void InsertHighlight(std::vector<Highlight>& highlights, const Highlight& highlight)
{
	std::vector<Highlight> newHighlights;
	newHighlights.reserve(highlights.size() + 2);

	auto it = highlights.begin();
	while (it != highlights.end() && it->begin < highlight.begin)
	{
		newHighlights.push_back(*it);
		++it;
	}

	while (it != highlights.end() && it->end <= highlight.end)
		++it;

	newHighlights.push_back(highlight);

	while (it != highlights.end())
	{
		newHighlights.push_back(*it);
		++it;
	}

	highlights.swap(newHighlights);
}

std::vector<Highlight> CLogView::GetHighlights(const std::string& text) const
{
	std::vector<Highlight> highlights;

	for (auto it = m_filter.messageFilters.begin(); it != m_filter.messageFilters.end(); ++it)
	{
		if (it->type != FilterType::Token)
			continue;

		std::sregex_iterator begin(text.begin(), text.end(), it->re), end;
		for (auto tok = begin; tok != end; ++tok)
			InsertHighlight(highlights, Highlight(tok->position(), tok->position() + tok->length(), it->bgColor, it->fgColor));
	}

	return highlights;
}

void DrawHighlightedText(HDC hdc, const RECT& rect, std::wstring text, std::vector<Highlight> highlights, const std::wstring& highlightText)
{
	auto line = boost::make_iterator_range(text);
	for (;;)
	{
		auto match = boost::algorithm::ifind_first(line, highlightText);
		if (match.empty())
			break;

		InsertHighlight(highlights, Highlight(match.begin() - text.begin(), match.end() - text.begin(), RGB(255, 255, 55), RGB(0, 0, 0)));
		line = boost::make_iterator_range(match.end(), line.end());
	}

	AddEllipsis(hdc, text, rect.right - rect.left);

	int height = GetTextSize(hdc, text, text.size()).cy;
	POINT pos = { rect.left, rect.top + (rect.bottom - rect.top - height)/2 };
	RECT rcHighlight = rect;
	for (auto it = highlights.begin(); it != highlights.end(); ++it)
	{
		rcHighlight.right = rect.left + GetTextSize(hdc, text, it->begin).cx;
		ExtTextOut(hdc, pos, rcHighlight, text);

		rcHighlight.left = rcHighlight.right;
		rcHighlight.right = rect.left + GetTextSize(hdc, text, it->end).cx;
		{
			ScopedTextColor txtcol(hdc, it->fgColor);
			ScopedBkColor bkcol(hdc, it->bkColor);
			ExtTextOut(hdc, pos, rcHighlight, text);
		}
		rcHighlight.left = rcHighlight.right;
	}
	rcHighlight.right = rect.right;
	ExtTextOut(hdc, pos, rcHighlight, text);
}

void CLogView::DrawBookmark(CDCHandle dc, int iItem) const
{
	if (!m_logLines[iItem].bookmark)
		return;
	RECT rect = GetSubItemRect(iItem, 0, LVIR_BOUNDS);
	dc.DrawIconEx(rect.left, rect.top, m_hBookmarkIcon.get(), 0, 0, 0, nullptr, DI_NORMAL | DI_COMPAT);
}

void CLogView::DrawSubItem(CDCHandle dc, int iItem, int iSubItem) const
{
	auto text = GetItemWText(iItem, iSubItem);
	RECT rect = GetSubItemRect(iItem, iSubItem, LVIR_BOUNDS);
	int margin = GetHeader().GetBitmapMargin();
	rect.left += margin;
	rect.right -= margin;
	if (iSubItem == 5)
		return DrawHighlightedText(dc, rect, text, m_logLines[iItem].highlights, m_highlightText);

	HDITEM item;
	item.mask = HDI_FORMAT;
	unsigned align = (GetHeader().GetItem(iSubItem, &item)) ? GetTextAlign(item) : HDF_LEFT;
	dc.DrawText(text.c_str(), text.size(), &rect, align | DT_NOCLIP | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

void CLogView::DrawItem(CDCHandle dc, int iItem, unsigned /*iItemState*/) const
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
	DrawBookmark(dc, iItem);
	for (int i = 1; i < subitemCount; ++i)
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
//		return CDRF_DODEFAULT;

	case CDDS_ITEMPREPAINT:
		DrawItem(pCustomDraw->nmcd.hdc, pCustomDraw->nmcd.dwItemSpec, pCustomDraw->nmcd.uItemState);
		return CDRF_SKIPDEFAULT;
	}

	return CDRF_DODEFAULT;
}

std::string TabsToSpaces(const std::string& s, int tabsize = 4)
{
	std::string result;
	result.reserve(s.size() + 3*tabsize);
	for (auto it = s.begin(); it != s.end(); ++it)
	{
		if (*it == '\t')
		{
			do
			{
				result.push_back(' ');
			}
			while (result.size() % tabsize != 0);
		}
		else
		{
			result.push_back(*it);
		}
	}
	return result;
}

template <typename CharT>
void CopyItemText(const CharT* s, wchar_t* buf, size_t maxLen)
{
	assert(maxLen > 0);

	for (int len = 0; len + 1U < maxLen && *s; ++s)
	{
		if (*s == '\t')
		{
			do
			{
				*buf++ = ' ';
				++len;
			}
			while (len + 1U < maxLen && len % 4 != 0);
		}
		else
		{
			*buf++ = *s;
			++len;
		}
	}

	*buf = '\0';
}

void CopyItemText(const std::string& s, wchar_t* buf, size_t maxLen)
{
	CopyItemText(s.c_str(), buf, maxLen);
}

void CopyItemText(const std::wstring& s, wchar_t* buf, size_t maxLen)
{
	CopyItemText(s.c_str(), buf, maxLen);
}

std::string GetTimeText(double time)
{
	return stringbuilder() << std::fixed << std::setprecision(6) << time;
}

std::string GetTimeText(const SYSTEMTIME& st)
{
	char buf[32];
	sprintf_s(buf, "%d:%02d:%02d.%03d", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	return buf;
}

std::string GetTimeText(const FILETIME& ft)
{
	return GetTimeText(FileTimeToSystemTime(FileTimeToLocalFileTime(ft)));
}

std::string CLogView::GetSubItemText(int iItem, int iSubItem) const
{
	int line = m_logLines[iItem].line;
	const Message& msg = m_logFile[line];

	switch (iSubItem)
	{
	case 1: return std::to_string(line + 1ULL);
	case 2: return m_clockTime ? GetTimeText(msg.systemTime) : GetTimeText(msg.time);
	case 3: return std::to_string(msg.processId + 0ULL);
	case 4: return Str(m_displayInfo.GetProcessName(msg.processId)).str();
	case 5: return msg.text;
	}
	return std::string();
}

LRESULT CLogView::OnGetDispInfo(NMHDR* pnmh)
{
	auto pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pnmh);
	LVITEM& item = pDispInfo->item;
	if ((item.mask & LVIF_TEXT) == 0 || item.iItem >= static_cast<int>(m_logLines.size()))
		return 0;

	CopyItemText(GetSubItemText(item.iItem, item.iSubItem), item.pszText, item.cchTextMax);
	return 0;
}

std::vector<int> CLogView::GetSelectedIndices() const
{
	std::vector<int> result;
	int item = -1;
	for (;;)
	{
		item = GetNextItem(item, LVNI_SELECTED);
		if (item < 0) 
			break;
		result.push_back(item);
	}
	return result;
}

SelectionInfo CLogView::GetSelectedRange() const
{
	int first = GetNextItem(-1, LVNI_SELECTED);
	if (first < 0)
		return SelectionInfo();

	int item = first;
	int last = first;
	do
	{
		last = item;
		item = GetNextItem(item, LVNI_SELECTED);
	}
	while (item > 0);

	return SelectionInfo(m_logLines[first].line, m_logLines[last].line, last - first + 1);
}

SelectionInfo CLogView::GetViewRange() const
{
	if (m_logLines.empty())
		return SelectionInfo();

	return SelectionInfo(m_logLines.front().line, m_logLines.back().line, m_logLines.size());
}

LRESULT CLogView::OnOdStateChanged(NMHDR* pnmh)
{
	auto& nmhdr = *reinterpret_cast<NMLVODSTATECHANGE*>(pnmh);
	nmhdr;

	return 0;
}

bool Contains(const std::string& text, const std::string& substring)
{
	return !boost::algorithm::ifind_first(text, substring).empty();
}

LRESULT CLogView::OnIncrementalSearch(NMHDR* pnmh)
{
	auto& nmhdr = *reinterpret_cast<NMLVFINDITEM*>(pnmh);

	std::string text(Str(nmhdr.lvfi.psz).str());
	m_mainFrame.SaitUpdate(WStr(text));
//	int line = nmhdr.iStart; // Does not work as specified...
	int line = std::max(GetNextItem(-1, LVNI_FOCUSED), 0);
	while (line != static_cast<int>(m_logLines.size()))
	{
		if (Contains(m_logFile[m_logLines[line].line].text, text))
		{
			SetHighlightText(nmhdr.lvfi.psz);
			nmhdr.lvfi.lParam = line;
			return 0;
		}
		++line;
	}

	nmhdr.lvfi.lParam = LVNSCH_ERROR;
	return 0;
}

LRESULT CLogView::OnOdCacheHint(NMHDR* pnmh)
{
	auto& nmhdr = *reinterpret_cast<NMLVCACHEHINT*>(pnmh);
	nmhdr;
	return 0;
}

void CLogView::OnViewClear(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	Clear();
}

void CLogView::OnViewSelectAll(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	SelectAll();
}

void CLogView::OnViewCopy(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	Copy();
}

void CLogView::OnViewScroll(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	SetScroll(!GetScroll());
}

void CLogView::OnViewTime(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	SetClockTime(!GetClockTime());
}

void CLogView::OnViewHideHighlight(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	SetHighlightText();
	m_mainFrame.SaitUpdate(L"");
	StopScrolling();
}

void CLogView::OnViewFindNext(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	if (!m_highlightText.empty())
		FindNext(m_highlightText);
}

void CLogView::OnViewFindPrevious(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	if (!m_highlightText.empty())
		FindPrevious(m_highlightText);
}

void CLogView::OnViewNextProcess(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
}

void CLogView::OnViewPreviousProcess(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
}

void CLogView::OnViewExclude(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	int item = GetNextItem(-1, LVIS_FOCUSED);
	if (item < 0)
		return;

	const auto& name = m_processInfo.GetProcessName(m_logFile[m_logLines[item].line].processId);
	m_filter.processFilters.push_back(ProcessFilter(Str(name), 0, FilterType::Exclude));
	ApplyFilters();
}

bool CLogView::GetBookmark() const
{
	int item = GetNextItem(-1, LVIS_FOCUSED);
	return item >= 0 && m_logLines[item].bookmark;
}

void CLogView::ToggleBookmark(int iItem)
{
	m_logLines[iItem].bookmark = !m_logLines[iItem].bookmark;
	auto rect = GetSubItemRect(iItem, 0, LVIR_BOUNDS);
	InvalidateRect(&rect);
}

void CLogView::OnViewBookmark(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	int item = GetNextItem(-1, LVIS_FOCUSED);
	if (item < 0)
		return;

	ToggleBookmark(item);
}

void CLogView::FindBookmark(int direction)
{
	int begin = std::max(GetNextItem(-1, LVNI_FOCUSED), 0);
	int line = begin;
	do
	{
		line += direction;
		if (line < 0)
			line += m_logLines.size();
		if (line == static_cast<int>(m_logLines.size()))
			line = 0;

		if (m_logLines[line].bookmark)
			return ScrollToIndex(line, false);
	}
	while (line != begin);
}

void CLogView::OnViewNextBookmark(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	FindBookmark(+1);
}

void CLogView::OnViewPreviousBookmark(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	FindBookmark(-1);
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
	if (enable)
		ScrollDown();
}

void CLogView::Clear()
{
	SetItemCount(0);
	m_dirty = false;
	m_logLines.clear();
	m_highlightText.clear();
}

void CLogView::Add(int line, const Message& msg)
{
	if (!IsIncluded(msg))
		return;

	m_dirty = true;
	m_logLines.push_back(LogLine(line, GetTextColor(msg.text)));
	m_logLines.back().highlights = GetHighlights(msg.text);
	if (m_autoScrollDown && IsStop(msg.text))
	{
		m_stop = [this, line] () 
		{ 
			StopScrolling();
			ScrollToIndex(line, true);
		};
		return;
	}

	if (IsTrack(msg.text))
	{
		m_track = [this, line] () 
		{ 
			ScrollToIndex(line, true);
		};
	}
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

	if (m_stop) 
	{
		m_stop();
		m_stop = 0;
	}
	if (m_track) 
	{
		m_track();
	}
}

void CLogView::StopScrolling()
{
	m_autoScrollDown = false;
	for (auto it = m_filter.messageFilters.begin(); it != m_filter.messageFilters.end(); ++it)
	{
		switch (it->type)
		{
		case FilterType::Track:
			it->enable = false;
			break;
		default:
			break;
		}
	}
	m_track = 0;
}

void CLogView::ClearSelection()
{
	auto lines = GetSelectedIndices();
	for (auto i = lines.begin(); i != lines.end(); ++i)
	{
		SetItemState(*i, static_cast<UINT>(~(LVIS_FOCUSED | LVIS_SELECTED)), LVIS_FOCUSED | LVIS_SELECTED);
	}
}

void CLogView::ScrollToIndex(int index, bool center)
{
	if (index < 0 || index >= static_cast<int>(m_logLines.size()))
		return;

	ClearSelection();
	SetItemState(index, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	EnsureVisible(index, false);

	if (center)
	{
		int maxExtraItems = GetCountPerPage() / 2;
		int maxBottomIndex = std::min<int>(m_logLines.size() - 1, index + maxExtraItems);
		EnsureVisible(maxBottomIndex, false);
	}
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
		GetItemText(item, 1) << "\t" <<
		GetItemText(item, 2) << "\t" <<
		GetItemText(item, 3) << "\t" <<
		GetItemText(item, 4) << "\t" <<
		GetItemText(item, 5);
}

void CLogView::Copy()
{
	std::ostringstream ss;

	if (!m_highlightText.empty())
	{
		ss << Str(m_highlightText);
	}
	else
	{
		int item = -1;
		while ((item = GetNextItem(item, LVNI_ALL | LVNI_SELECTED)) >= 0)
			ss << GetItemText(item) << "\n";
	}
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

void CLogView::SetHighlightText(const std::wstring& text)
{
	m_highlightText = text;
	Invalidate(false);
}

bool CLogView::Find(const std::string& text, int direction)
{
	int begin = std::max(GetNextItem(-1, LVNI_FOCUSED), 0);
	int line = begin;
	while (line != begin)
	{
		line += direction;
		if (line < 0)
			line += m_logLines.size();
		if (line == static_cast<int>(m_logLines.size()))
			line = 0;

		if (Contains(m_logFile[m_logLines[line].line].text, text))
		{
			ScrollToIndex(line, true);
			SetHighlightText(WStr(text));
			return true;
		}
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
	SetClockTime(RegGetDWORDValue(reg, L"ClockTime", 1) != 0);

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

		m_filter.messageFilters.push_back(MessageFilter(
			Str(RegGetStringValue(regFilter)),
			IntToFilterType(RegGetDWORDValue(regFilter, L"Type")),
			RegGetDWORDValue(regFilter, L"BgColor", RGB(255, 255, 255)),
			RegGetDWORDValue(regFilter, L"FgColor", RGB(0, 0, 0)),
			RegGetDWORDValue(regFilter, L"Enable", 1) != 0));
	}

	for (size_t i = 0; ; ++i)
	{
		CRegKey regFilter;
		if (regFilter.Open(reg, WStr(wstringbuilder() << L"ProcessFilters\\Filter" << i)) != ERROR_SUCCESS)
			break;

		m_filter.processFilters.push_back(ProcessFilter(
			Str(RegGetStringValue(regFilter)),
			0,
			IntToFilterType(RegGetDWORDValue(regFilter, L"Type")),
			RegGetDWORDValue(regFilter, L"BgColor", RGB(255, 255, 255)),
			RegGetDWORDValue(regFilter, L"FgColor", RGB(0, 0, 0)),
			RegGetDWORDValue(regFilter, L"Enable", 1) != 0));
	}

	ApplyFilters();
}

void CLogView::SaveSettings(CRegKey& reg)
{
	std::wostringstream ss;
	reg.SetDWORDValue(L"ClockTime", GetClockTime());
	int subitemCount = GetHeader().GetItemCount();
	for (int i = 0; i < subitemCount; ++i)
		ss << GetColumnWidth(i) << " ";
	reg.SetStringValue(L"ColWidths", ss.str().c_str());

	int i = 0;
	for (auto it = m_filter.messageFilters.begin(); it != m_filter.messageFilters.end(); ++it, ++i)
	{
		CRegKey regFilter;
		regFilter.Create(reg, WStr(wstringbuilder() << L"Filters\\Filter" << i));
		regFilter.SetStringValue(L"", WStr(it->text.c_str()));
		regFilter.SetDWORDValue(L"Type", FilterTypeToInt(it->type));
		regFilter.SetDWORDValue(L"BgColor", it->bgColor);
		regFilter.SetDWORDValue(L"FgColor", it->fgColor);
		regFilter.SetDWORDValue(L"Enable", it->enable);
	}

	i = 0;
	for (auto it = m_filter.processFilters.begin(); it != m_filter.processFilters.end(); ++it, ++i)
	{
		CRegKey regFilter;
		regFilter.Create(reg, WStr(wstringbuilder() << L"ProcessFilters\\Filter" << i));
		regFilter.SetStringValue(L"", WStr(it->text.c_str()));
		regFilter.SetDWORDValue(L"Type", FilterTypeToInt(it->type));
		regFilter.SetDWORDValue(L"BgColor", it->bgColor);
		regFilter.SetDWORDValue(L"FgColor", it->fgColor);
		regFilter.SetDWORDValue(L"Enable", it->enable);
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

LogFilter CLogView::GetFilters() const
{
	return m_filter;
}

void CLogView::SetFilters(const LogFilter& filter)
{
	m_track = 0;
	m_filter = filter;
	ApplyFilters();
}

void CLogView::ApplyFilters()
{
	SetItemCount(0);
	m_logLines.clear();

	int count = m_logFile.Count();
	for (int i = 0; i < count; ++i)
		Add(i, m_logFile[i]);

	EndUpdate();
}

TextColor CLogView::GetTextColor(const std::string& text) const
{
	for (auto it = m_filter.messageFilters.begin(); it != m_filter.messageFilters.end(); ++it)
	{
		if (it->enable && it->type == FilterType::Highlight && std::regex_search(text, it->re))
			return TextColor(it->bgColor, it->fgColor);
	}

	for (auto it = m_filter.processFilters.begin(); it != m_filter.processFilters.end(); ++it)
	{
		if (it->enable && it->type == FilterType::Highlight && std::regex_search(text, it->re))
			return TextColor(it->bgColor, it->fgColor);
	}

	return TextColor(GetSysColor(COLOR_WINDOW), GetSysColor(COLOR_WINDOWTEXT));
}

bool CLogView::IsProcessIncluded(const std::wstring& process) const
{
	std::string text = Str(process).str();

	for (auto it = m_filter.processFilters.begin(); it != m_filter.processFilters.end(); ++it)
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

bool CLogView::IsMessageIncluded(const std::string& text) const
{
	for (auto it = m_filter.messageFilters.begin(); it != m_filter.messageFilters.end(); ++it)
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

bool CLogView::IsIncluded(const Message& msg) const
{
	return IsProcessIncluded(m_processInfo.GetProcessName(msg.processId)) && IsMessageIncluded(msg.text);
}

bool CLogView::IsStop(const std::string& text) const
{
	for (auto it = m_filter.messageFilters.begin(); it != m_filter.messageFilters.end(); ++it)
	{
		if (!it->enable)
			continue;

		switch (it->type)
		{
		case FilterType::Stop:
			return std::regex_search(text, it->re);

		default:
			break;
		}
	}
	return false;
}

bool CLogView::IsTrack(const std::string& text) const
{
	for (auto it = m_filter.messageFilters.begin(); it != m_filter.messageFilters.end(); ++it)
	{
		if (!it->enable)
			continue;

		switch (it->type)
		{
		case FilterType::Track:
			return std::regex_search(text, it->re);

		default:
			break;
		}
	}
	return false;
}

} // namespace fusion
