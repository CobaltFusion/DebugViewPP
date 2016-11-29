// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <iomanip>
#include <array>
#include <regex>
#include <unordered_set>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include "CobaltFusion/AtlWinExt.h"
#include "CobaltFusion/stringbuilder.h"
#include "CobaltFusion/dbgstream.h"
#include "Win32/Registry.h"
#include "DebugView++Lib/Conversions.h"
#include "DebugView++Lib/FileIO.h"
#include "Resource.h"
#include "MainFrame.h"
#include "LogView.h"

namespace fusion {
namespace debugviewpp {

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

int GetTextOffset(HDC hdc, const std::string& s, int xPos)
{
	auto exp = TabsToSpaces(s);
	int nFit;
	SIZE size;
	if (!GetTextExtentExPointA(hdc, exp.c_str(), exp.size(), xPos, &nFit, nullptr, &size))
		return 0;
	return SkipTabOffset(s, nFit);
}

int GetTextOffset(HDC hdc, const std::wstring& s, int xPos)
{
	auto exp = TabsToSpaces(s);
	int nFit;
	SIZE size;
	if (xPos <= 0 || !GetTextExtentExPointW(hdc, exp.c_str(), exp.size(), xPos, &nFit, nullptr, &size))
		return 0;
	return SkipTabOffset(s, nFit);
}

void AddEllipsis(HDC hdc, std::wstring& text, int width)
{
	static const std::wstring ellipsis(L"...");
	int pos = GetTextOffset(hdc, text, width);
	if (pos >= 0 && pos < static_cast<int>(text.size()))
	{
		pos = GetTextOffset(hdc, text, width - GetTextSize(hdc, ellipsis, ellipsis.size()).cx);
		text = text.substr(0, pos) + ellipsis;
	}
}

SelectionInfo::SelectionInfo() :
	beginLine(0), endLine(0), count(0)
{
}

SelectionInfo::SelectionInfo(int beginLine, int endLine, int count) :
	beginLine(beginLine), endLine(endLine), count(count)
{
}

TextColor::TextColor(COLORREF back, COLORREF fore) :
	back(back), fore(fore)
{
}

Highlight::Highlight(int id, int begin, int end, const TextColor& color) :
	id(id), begin(begin), end(end), color(color)
{
}

LogLine::LogLine(int line) :
	bookmark(false), line(line)
{
}

ItemData::ItemData() :
	color(Colors::BackGround, Colors::Text)
{
}

BEGIN_MSG_MAP2(CLogView)
	MSG_WM_CREATE(OnCreate)
	MSG_WM_DROPFILES(OnDropFiles)
	MSG_WM_CONTEXTMENU(OnContextMenu)
	MSG_WM_SETCURSOR(OnSetCursor)
	MSG_WM_LBUTTONDOWN(OnLButtonDown)
	MSG_WM_MOUSEMOVE(OnMouseMove)
	MSG_WM_LBUTTONUP(OnLButtonUp)
	MSG_WM_TIMER(OnTimer)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(NM_CLICK, OnClick)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(NM_DBLCLK, OnDblClick)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_ITEMCHANGED, OnItemChanged)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_GETDISPINFO, OnGetDispInfo)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_ODSTATECHANGED, OnOdStateChanged)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_INCREMENTALSEARCH, OnIncrementalSearch)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_ODCACHEHINT, OnOdCacheHint)
	REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_BEGINDRAG, OnBeginDrag)
	COMMAND_ID_HANDLER_EX(ID_VIEW_CLEAR, OnViewClear)
	COMMAND_ID_HANDLER_EX(ID_VIEW_EXCLUDE_LINES, OnViewExcludeLines)
	COMMAND_ID_HANDLER_EX(ID_VIEW_RESET, OnViewReset)
	COMMAND_ID_HANDLER_EX(ID_VIEW_RESET_TO_LINE, OnViewResetToLine)
	COMMAND_ID_HANDLER_EX(ID_VIEW_SELECTALL, OnViewSelectAll)
	COMMAND_ID_HANDLER_EX(ID_VIEW_COPY, OnViewCopy)
	COMMAND_ID_HANDLER_EX(ID_VIEW_SCROLL, OnViewAutoScroll)
	COMMAND_ID_HANDLER_EX(ID_VIEW_SCROLL_STOP, OnViewAutoScrollStop)
	COMMAND_ID_HANDLER_EX(ID_VIEW_TIME, OnViewTime)
	COMMAND_ID_HANDLER_EX(ID_VIEW_PROCESSCOLORS, OnViewProcessColors);
	COMMAND_ID_HANDLER_EX(ID_VIEW_HIDE_HIGHLIGHT, OnEscapeKey)
	COMMAND_ID_HANDLER_EX(ID_VIEW_FIND_NEXT, OnViewFindNext)
	COMMAND_ID_HANDLER_EX(ID_VIEW_FIND_PREVIOUS, OnViewFindPrevious)
	COMMAND_ID_HANDLER_EX(ID_VIEW_NEXT_PROCESS, OnViewNextProcess)
	COMMAND_ID_HANDLER_EX(ID_VIEW_PREVIOUS_PROCESS, OnViewPreviousProcess)
	COMMAND_ID_HANDLER_EX(ID_VIEW_PROCESS_HIGHLIGHT, OnViewProcessHighlight)
	COMMAND_ID_HANDLER_EX(ID_VIEW_PROCESS_INCLUDE, OnViewProcessInclude)
	COMMAND_ID_HANDLER_EX(ID_VIEW_PROCESS_EXCLUDE, OnViewProcessExclude)
	COMMAND_ID_HANDLER_EX(ID_VIEW_PROCESS_TRACK, OnViewProcessTrack)
	COMMAND_ID_HANDLER_EX(ID_VIEW_PROCESS_ONCE, OnViewProcessOnce)
	COMMAND_ID_HANDLER_EX(ID_VIEW_FILTER_HIGHLIGHT, OnViewFilterHighlight)
	COMMAND_ID_HANDLER_EX(ID_VIEW_FILTER_INCLUDE, OnViewFilterInclude)
	COMMAND_ID_HANDLER_EX(ID_VIEW_FILTER_EXCLUDE, OnViewFilterExclude)
	COMMAND_ID_HANDLER_EX(ID_VIEW_FILTER_TOKEN, OnViewFilterToken)
	COMMAND_ID_HANDLER_EX(ID_VIEW_FILTER_TRACK, OnViewFilterTrack)
	COMMAND_ID_HANDLER_EX(ID_VIEW_FILTER_ONCE, OnViewFilterOnce)
	COMMAND_ID_HANDLER_EX(ID_VIEW_BOOKMARK, OnViewBookmark)
	COMMAND_ID_HANDLER_EX(ID_VIEW_NEXT_BOOKMARK, OnViewNextBookmark)
	COMMAND_ID_HANDLER_EX(ID_VIEW_PREVIOUS_BOOKMARK, OnViewPreviousBookmark)
	COMMAND_ID_HANDLER_EX(ID_VIEW_CLEAR_BOOKMARKS, OnViewClearBookmarks)
	COMMAND_RANGE_HANDLER_EX(ID_VIEW_COLUMN_FIRST, ID_VIEW_COLUMN_LAST, OnViewColumn)
	CHAIN_MSG_MAP_ALT(COwnerDraw<CLogView>, 1)
	CHAIN_MSG_MAP(CDoubleBufferImpl<CLogView>)		//DrMemory: GDI USAGE ERROR: DC 0x3e011cca that contains selected object being deleted
	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()

bool CLogView::IsColumnViewed(int nID) const
{
	return m_columns[nID - ID_VIEW_COLUMN_FIRST].enable;
}

void CLogView::OnViewColumn(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
	UpdateColumnInfo();

	auto& column = m_columns[nID - ID_VIEW_COLUMN_FIRST];
	column.enable = !column.enable;

	int delta = column.enable ? +1 : -1;
	for (auto& col : m_columns)
		if (col.column.iSubItem != column.column.iSubItem && col.column.iOrder >= column.column.iOrder)
			col.column.iOrder += delta;

	UpdateColumns();
}

CLogView::CLogView(const std::wstring& name, CMainFrame& mainFrame, LogFile& logFile, LogFilter filter) :
	m_name(name),
	m_mainFrame(mainFrame),
	m_logFile(logFile),
	m_filter(std::move(filter)),
	m_firstLine(0),
	m_clockTime(false),
	m_processColors(false),
	m_autoScrollDown(true),
	m_autoScrollStop(true),
	m_dirty(false),
	m_hBookmarkIcon(static_cast<HICON>(LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_BOOKMARK), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR))),
	m_hBeamCursor(LoadCursor(nullptr, IDC_IBEAM)),
	m_dragStart(0, 0),
	m_dragEnd(0, 0),
	m_dragging(false),
	m_scrollX(0)
{
}

void CLogView::OnException()
{
	MessageBox(L"Unknown Exception", LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
}

void CLogView::OnException(const std::exception& ex)
{
	MessageBox(WStr(ex.what()).c_str(), LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
}

int CLogView::ColumnToSubItem(Column::type iColumn) const
{
	int columns = GetHeader().GetItemCount();
	for (int iSubItem = 0; iSubItem < columns; ++iSubItem)
	{
		LVCOLUMN column;
		column.mask = LVCF_SUBITEM;
		GetColumn(iSubItem, &column);
		if (column.iSubItem == iColumn)
			return iSubItem;
	}
	return 0;
}

Column::type CLogView::SubItemToColumn(int iSubItem) const
{
	LVCOLUMN column;
	column.mask = LVCF_SUBITEM;
	GetColumn(iSubItem, &column);
	return static_cast<Column::type>(column.iSubItem);
}

void CLogView::UpdateColumnInfo()
{
	int count = GetHeader().GetItemCount();
	for (int i = 0; i < count; ++i)
	{
		auto& column = m_columns[SubItemToColumn(i)].column;
		auto column2 = column;
		column2.mask = LVCF_WIDTH | LVCF_ORDER;
		GetColumn(i, &column2);
		column.cx = column2.cx;
		column.iOrder = column2.iOrder;
	}
}

void CLogView::UpdateColumns()
{
	int columns = GetHeader().GetItemCount();
	for (int i = 0; i < columns; ++i)
		DeleteColumn(0);

	int col = 0;
	for (auto& item : m_columns)
	{
		if (item.enable)
			InsertColumn(col++, &item.column);
	}
}

ColumnInfo MakeColumn(Column::type column, const wchar_t* name, int format, int width)
{
	ColumnInfo info;
	info.enable = true;
	info.column.iSubItem = column;
	info.column.iOrder = column;
	info.column.pszText = const_cast<wchar_t*>(name);
	info.column.fmt = format;
	info.column.cx = width;
	info.column.mask = LVCF_SUBITEM | LVCF_ORDER | LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
	return info;
}

LRESULT CLogView::OnCreate(const CREATESTRUCT* /*pCreate*/)
{
	DefWindowProc();

	SetExtendedListViewStyle(GetWndExStyle(0));
	m_hdr.SubclassWindow(GetHeader());

	m_columns.push_back(MakeColumn(Column::Bookmark, L"", LVCFMT_RIGHT, 20));
	m_columns.push_back(MakeColumn(Column::Line, L"Line", LVCFMT_RIGHT, 60));
	m_columns.push_back(MakeColumn(Column::Date, L"Date", LVCFMT_RIGHT, 90));
	m_columns.back().enable = false; // Default no Date column
	m_columns.push_back(MakeColumn(Column::Time, L"Time", LVCFMT_RIGHT, 90));
	m_columns.push_back(MakeColumn(Column::Pid, L"PID", LVCFMT_RIGHT, 60));
	m_columns.push_back(MakeColumn(Column::Process, L"Process", LVCFMT_LEFT, 140));
	m_columns.push_back(MakeColumn(Column::Message, L"Message", LVCFMT_LEFT, 1500));
	UpdateColumns();

	ApplyFilters();

	return 0;
}

bool Contains(const RECT& rect, const POINT& pt)
{
	return pt.x >= rect.left && pt.x < rect.right && pt.y >= rect.top && pt.y < rect.bottom;
}

BOOL CLogView::OnSetCursor(CWindow /*wnd*/, UINT /*nHitTest*/, UINT /*message*/)
{
	POINT pt = Win32::GetMessagePos();
	ScreenToClient(&pt);

	RECT client;
	GetClientRect(&client);
	if (!Contains(client, pt))
	{
		SetMsgHandled(false);
		return FALSE;
	}

	LVHITTESTINFO info;
	info.flags = 0;
	info.pt = pt;
	SubItemHitTest(&info);
	if ((info.flags & LVHT_ONITEM) != 0 && info.iSubItem == ColumnToSubItem(Column::Message))
	{
		::SetCursor(m_hBeamCursor);
		return TRUE;
	}

	SetMsgHandled(false);
	return FALSE;
}

int CLogView::TextHighlightHitTest(int iItem, const POINT& pt)
{
	int pos = GetTextIndex(iItem, pt.x);
	auto highlights = GetItemData(iItem).highlights;
	auto it = highlights.begin();
	while (it != highlights.end() && it->end <= pos)
		++it;
	if (it != highlights.end() && it->begin <= pos)
		return it->id;
	return 0;
}

void CLogView::OnDropFiles(HDROP hDropInfo)
{
	// Need to maunally forward to CMainFrame, don't understand why
	m_mainFrame.OnDropFiles(hDropInfo);
}

std::vector<std::string> CLogView::GetSelectedMessages() const
{
	std::vector<std::string> messages;
	int item = -1;
	while ((item = GetNextItem(item, LVNI_ALL | LVNI_SELECTED)) >= 0)
		messages.push_back(GetColumnText(item, Column::Message));
	return messages;
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

	HDHITTESTINFO hdrInfo;
	hdrInfo.flags = 0;
	hdrInfo.pt = pt;
	GetHeader().HitTest(&hdrInfo);

	LVHITTESTINFO info;
	info.flags = 0;
	info.pt = pt;
	SubItemHitTest(&info);

	int menuId = 0;
	if ((hdrInfo.flags & HHT_ONHEADER) != 0)
		menuId = IDR_HEADER_CONTEXTMENU;
	else if ((info.flags & LVHT_ONITEM) != 0)
	{
		switch (SubItemToColumn(info.iSubItem))
		{
		case Column::Process:
			menuId = IDR_PROCESS_CONTEXTMENU;
			break;
		case Column::Message:
			menuId = TextHighlightHitTest(info.iItem, pt) == 1 ? IDR_HIGHLIGHT_CONTEXTMENU : IDR_VIEW_CONTEXTMENU;
			break;
		default:	
			menuId = IDR_VIEW_CONTEXTMENU;
			break;
		}
	}
	else
		return;

	CMenu menuContext;
	menuContext.LoadMenu(menuId);
	CMenuHandle menuPopup(menuContext.GetSubMenu(0));
	ClientToScreen(&pt);
	menuPopup.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_mainFrame);
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
	if ((info.flags & LVHT_ONITEM) != 0 && info.iSubItem == ColumnToSubItem(Column::Bookmark))
		ToggleBookmark(info.iItem);

	return 0;
}

void CLogView::OnLButtonDown(UINT flags, CPoint point)
{
	if ((flags & MK_SHIFT) == 0 || m_highlightText.empty())
	{
		SetMsgHandled(false);
		return;
	}

	LVHITTESTINFO info;
	info.flags = 0;
	info.pt = point;
	SubItemHitTest(&info);
	if ((info.flags & LVHT_ONITEM) == 0 || info.iSubItem != ColumnToSubItem(Column::Message))
	{
		SetMsgHandled(false);
		return;
	}

	CClientDC dc(*this);
	Win32::GdiObjectSelection font(dc, GetFont());

	int x0 = GetSubItemRect(info.iItem, info.iSubItem, LVIR_BOUNDS).left + GetHeader().GetBitmapMargin();
	auto line = TabsToSpaces(GetItemWText(info.iItem, ColumnToSubItem(Column::Message)));
	auto pos = 0;
	int min = 1000 * 1000;
	bool found = false;
	for (;;)
	{
		pos = line.find(m_highlightText, pos);
		if (pos == line.npos)
			break;

		int x1 = x0 + GetTextSize(dc.m_hDC, line, pos).cx;
		int x2 = x0 + GetTextSize(dc.m_hDC, line, pos + m_highlightText.size()).cx;
		if (std::abs(point.x - x1) < min)
		{
			min = std::abs(point.x - x1);
			m_dragStart = CPoint(x2, point.y);
		}
		if (std::abs(point.x - x2) < min)
		{
			min = std::abs(point.x - x2);
			m_dragStart = CPoint(x1, point.y);
		}

		pos += m_highlightText.size();
		found = true;
	}
	if (!found)
	{
		SetMsgHandled(false);
		return;
	}

	StopTracking();

	SetCapture();
	m_dragging = true;
	m_dragEnd = point;
	m_dragStart.x += GetScrollPos(SB_HORZ);
	m_dragEnd.x += GetScrollPos(SB_HORZ);
	Invalidate();
}

void CLogView::OnMouseMove(UINT /*flags*/, CPoint point)
{
	SetMsgHandled(false);
	if (!m_dragging)
		return;

	m_dragEnd = point;
	m_dragEnd.x += GetScrollPos(SB_HORZ);
	Invalidate();

	RECT rect;
	GetClientRect(&rect);
	if (point.x < rect.left + 32)
	{
		if (m_scrollX == 0)
			SetTimer(1, 25, nullptr);
		m_scrollX = -8;
	}
	else if (point.x > rect.right - 32)
	{
		if (m_scrollX == 0)
			SetTimer(1, 25, nullptr);
		m_scrollX = +8;
	}
	else
	{
		if (m_scrollX != 0)
		{
			KillTimer(1);
			m_scrollX = 0;
		}
	}
}

void CLogView::OnLButtonUp(UINT /*flags*/, CPoint point)
{
	SetMsgHandled(false);

	if (m_scrollX)
		KillTimer(1);

	if (!m_dragging)
		return;

	m_dragging = false;

	auto dragStart = m_dragStart;
	dragStart.x -= GetScrollPos(SB_HORZ);

	if (abs(point.x - dragStart.x) <= GetSystemMetrics(SM_CXDRAG) &&
		abs(point.y - dragStart.y) <= GetSystemMetrics(SM_CYDRAG))
		return;

	LVHITTESTINFO info;
	info.flags = 0;
	info.pt = dragStart;
	SubItemHitTest(&info);
	int x1 = std::min(dragStart.x, point.x);
	int x2 = std::max(dragStart.x, point.x);
	m_dragStart = CPoint();
	m_dragEnd = CPoint();
	ReleaseCapture();
	Invalidate();

	if ((info.flags & LVHT_ONITEM) == 0 || SubItemToColumn(info.iSubItem) != Column::Message)
		return;

	int begin = GetTextIndex(info.iItem, x1);
	int end = GetTextIndex(info.iItem, x2);
	SetHighlightText(TabsToSpaces(GetItemWText(info.iItem, ColumnToSubItem(Column::Message))).substr(begin, end - begin));
}

void CLogView::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent != 1)
		return;

	Scroll(CSize(m_scrollX, 0));
}

void CLogView::MeasureItem(MEASUREITEMSTRUCT* pMeasureItemStruct)
{
	CClientDC dc(*this);

	Win32::GdiObjectSelection font(dc, GetFont());
	TEXTMETRIC metric;
	dc.GetTextMetrics(&metric);
	pMeasureItemStruct->itemHeight = metric.tmHeight;
}

void CLogView::DrawItem(DRAWITEMSTRUCT* pDrawItemStruct)
{
	DrawItem(pDrawItemStruct->hDC, pDrawItemStruct->itemID, pDrawItemStruct->itemState);
}

void CLogView::DeleteItem(DELETEITEMSTRUCT* lParam)
{
	COwnerDraw<CLogView>::DeleteItem(lParam);
}

int CLogView::GetTextIndex(int iItem, int xPos)
{
	CClientDC dc(*this);
	Win32::GdiObjectSelection font(dc, GetFont());
	return GetTextIndex(dc.m_hDC, iItem, xPos);
}

int CLogView::GetTextIndex(CDCHandle dc, int iItem, int xPos) const
{
	auto rect = GetSubItemRect(0, ColumnToSubItem(Column::Message), LVIR_BOUNDS);
	int x0 = rect.left + GetHeader().GetBitmapMargin();

	auto text = GetItemWText(iItem, ColumnToSubItem(Column::Message));
	return GetTextOffset(dc, text, xPos - x0);
}

LRESULT CLogView::OnDblClick(NMHDR* pnmh)
{
	auto& nmhdr = *reinterpret_cast<NMITEMACTIVATE*>(pnmh);

	if (SubItemToColumn(nmhdr.iSubItem) != Column::Message || nmhdr.iItem < 0 || static_cast<size_t>(nmhdr.iItem) >= m_logLines.size())
		return 0;

	int nFit = GetTextIndex(nmhdr.iItem, nmhdr.ptAction.x);
	auto text = TabsToSpaces(GetItemWText(nmhdr.iItem, ColumnToSubItem(Column::Message)));

	int begin = nFit;
	while (begin > 0)
	{
		if (!iswordchar(text[begin - 1]))
			break;
		--begin;
	}
	int end = nFit;
	while (end < static_cast<int>(text.size()))
	{
		if (!iswordchar(text[end]))
			break;
		++end;
	}
	SetHighlightText(std::wstring(text.begin() + begin, text.begin() + end));
	return 0;
}

LRESULT CLogView::OnItemChanged(NMHDR* pnmh)
{
	auto& nmhdr = *reinterpret_cast<NMLISTVIEW*>(pnmh);

	if ((nmhdr.uNewState & LVIS_FOCUSED) == 0 ||
		nmhdr.iItem < 0  ||
		static_cast<size_t>(nmhdr.iItem) >= m_logLines.size())
		return 0;

	if (m_autoScrollStop)
		m_autoScrollDown = nmhdr.iItem == GetItemCount() - 1;

	SetHighlightText();
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
	RECT rect;
	CListViewCtrl::GetSubItemRect(iItem, iSubItem, code, &rect);
	if (iSubItem == 0)
		rect.right = rect.left + GetColumnWidth(0);
	return rect;
}

void InsertHighlight(std::vector<Highlight>& highlights, const Highlight& highlight)
{
	if (highlight.begin == highlight.end)
		return;

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

void InsertHighlight(std::vector<Highlight>& highlights, const std::string& text, const std::string& match, TextColor color)
{
	auto line = boost::make_iterator_range(text);
	for (;;)
	{
		auto range = boost::algorithm::ifind_first(line, match);
		if (range.empty())
			break;
		int begin = ExpandedTabOffset(text, range.begin() - text.begin());
		int end = ExpandedTabOffset(text, range.end() - text.begin());
		InsertHighlight(highlights, Highlight(1, begin, end, color));
		line = boost::make_iterator_range(range.end(), line.end());
	}
}

std::vector<Highlight> CLogView::GetHighlights(const std::string& text) const
{
	std::vector<Highlight> highlights;

	int highlightId = 1;
	for (auto& filter : m_filter.messageFilters)
	{
		if (!filter.enable || filter.filterType != FilterType::Token)
			continue;

		std::sregex_iterator begin(text.begin(), text.end(), filter.re), end;
		int id = ++highlightId;
		for (auto tok = begin; tok != end; ++tok)
		{
			int first = 0;
			int count = 1;
			if (tok->size() > 1 && filter.matchType == MatchType::RegexGroups)
			{
				first = 1;
				count = tok->size();
			}
			for (int i = first; i < count; ++i)
			{
				int beginIndex = ExpandedTabOffset(text, tok->position(i));
				int endIndex = ExpandedTabOffset(text, tok->position(i) + tok->length(i));

				if (filter.bgColor == Colors::Auto)
				{
					auto itc = m_matchColors.find(tok->str(i));
					if (itc != m_matchColors.end())
						InsertHighlight(highlights, Highlight(id, beginIndex, endIndex, TextColor(itc->second, Colors::Text)));
				}
				else
				{
					InsertHighlight(highlights, Highlight(id, beginIndex, endIndex, TextColor(filter.bgColor, filter.fgColor)));
				}
			}
		}
	}

	InsertHighlight(highlights, text, Str(m_highlightText), TextColor(Colors::Highlight, Colors::Text));

	return highlights;
}

void DrawHighlightedText(HDC hdc, const RECT& rect, std::wstring text, std::vector<Highlight> highlights, const Highlight& selection)
{
	InsertHighlight(highlights, selection);

	AddEllipsis(hdc, text, rect.right - rect.left);

	int height = GetTextSize(hdc, text, text.size()).cy;
	POINT pos = { rect.left, rect.top + (rect.bottom - rect.top - height)/2 };
	RECT rcHighlight = rect;
	for (auto& highlight : highlights)
	{
		rcHighlight.right = rect.left + GetTextSize(hdc, text, highlight.begin).cx;
		ExtTextOut(hdc, pos, rcHighlight, text);

		rcHighlight.left = rcHighlight.right;
		rcHighlight.right = rect.left + GetTextSize(hdc, text, highlight.end).cx;
		{
			Win32::ScopedTextColor txtcol(hdc, highlight.color.fore);
			Win32::ScopedBkColor bkcol(hdc, highlight.color.back);
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
	dc.DrawIconEx(rect.left /* + GetHeader().GetBitmapMargin() */, rect.top + (rect.bottom - rect.top - 16)/2, m_hBookmarkIcon.get(), 0, 0, 0, nullptr, DI_NORMAL | DI_COMPAT);
}

ItemData CLogView::GetItemData(int iItem) const
{
	ItemData data;
	data.text[Column::Line] = GetItemWText(iItem, ColumnToSubItem(Column::Line));
	data.text[Column::Date] = GetItemWText(iItem, ColumnToSubItem(Column::Date));
	data.text[Column::Time] = GetItemWText(iItem, ColumnToSubItem(Column::Time));
	data.text[Column::Pid] = GetItemWText(iItem, ColumnToSubItem(Column::Pid));
	data.text[Column::Process] = GetItemWText(iItem, ColumnToSubItem(Column::Process));
	auto text = TabsToSpaces(m_logFile[m_logLines[iItem].line].text);
	data.highlights = GetHighlights(m_logFile[m_logLines[iItem].line].text);
	data.text[Column::Message] = WStr(text).str();
	data.color = GetTextColor(m_logFile[m_logLines[iItem].line]);
	return data;
}

Highlight CLogView::GetSelectionHighlight(CDCHandle dc, int iItem) const
{
	auto rect = GetSubItemRect(iItem, ColumnToSubItem(Column::Message), LVIR_BOUNDS);
	auto dragStart = m_dragStart;
	dragStart.x -=  GetScrollPos(SB_HORZ);
	auto dragEnd = m_dragEnd;
	dragEnd.x -=  GetScrollPos(SB_HORZ);
	if (!m_dragging || !Contains(rect, dragStart))
		return Highlight(0, 0, 0, TextColor(0, 0));

	int x1 = std::min(dragStart.x, dragEnd.x);
	int x2 = std::max(dragStart.x, dragEnd.x);

	int begin = GetTextIndex(dc, iItem, x1);
	int end = GetTextIndex(dc, iItem, x2);
	return Highlight(0, begin, end, TextColor(Colors::Selection, Colors::Text));
}

void CLogView::DrawSubItem(CDCHandle dc, int iItem, int iSubItem, const ItemData& data) const
{
	auto column = SubItemToColumn(iSubItem);
	const auto& text = data.text[column];
	RECT rect = GetSubItemRect(iItem, iSubItem, LVIR_BOUNDS);
	int margin = GetHeader().GetBitmapMargin();
	rect.left += margin;
	rect.right -= margin;
	if (column == Column::Message)
		return DrawHighlightedText(dc, rect, text, data.highlights, GetSelectionHighlight(dc, iItem));

	HDITEM item;
	item.mask = HDI_FORMAT;
	unsigned align = (GetHeader().GetItem(iSubItem, &item)) ? GetTextAlign(item) : DT_LEFT;
	dc.DrawText(text.c_str(), text.size(), &rect, align | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

void CLogView::DrawItem(CDCHandle dc, int iItem, unsigned /*iItemState*/) const
{
	auto rect = GetItemRect(iItem, LVIR_BOUNDS);
	auto data = GetItemData(iItem);

	bool selected = GetItemState(iItem, LVIS_SELECTED) == LVIS_SELECTED;
	bool focused = GetItemState(iItem, LVIS_FOCUSED) == LVIS_FOCUSED;
	auto bkColor = selected ? Colors::ItemHighlight : data.color.back;
	auto txColor = selected ? Colors::ItemHighlightText : data.color.fore;

	rect.left += GetColumnWidth(0);
	dc.FillSolidRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, bkColor);

	Win32::ScopedBkColor bcol(dc, bkColor);
	Win32::ScopedTextColor tcol(dc, txColor);

	int subitemCount = GetHeader().GetItemCount();
	DrawBookmark(dc, iItem);
	for (int i = 1; i < subitemCount; ++i)
		DrawSubItem(dc, iItem, i, data);
	if (focused)
		dc.DrawFocusRect(&rect);
}

std::string CLogView::GetColumnText(int iItem, Column::type column) const
{
	int line = m_logLines[iItem].line;
	const Message& msg = m_logFile[line];

	switch (column)
	{
	case Column::Line: return std::to_string(iItem + 1ULL);
	case Column::Date: return GetDateText(msg.systemTime);
	case Column::Time: return m_clockTime ? GetTimeText(msg.systemTime) : GetTimeText(msg.time);
	case Column::Pid: return std::to_string(msg.processId + 0ULL);
	case Column::Process: return Str(msg.processName).str();
	case Column::Message: return msg.text;
	}
	return std::string();
}

LRESULT CLogView::OnGetDispInfo(NMHDR* pnmh)
{
	auto pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pnmh);
	LVITEM& item = pDispInfo->item;
	if ((item.mask & LVIF_TEXT) == 0 || item.iItem >= static_cast<int>(m_logLines.size()))
		return 0;

	m_dispInfoText = WStr(GetColumnText(item.iItem, SubItemToColumn(item.iSubItem))).str();
	item.pszText = const_cast<wchar_t*>(m_dispInfoText.c_str());
	return 0;
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
	Win32::ScopedCursor cursor(::LoadCursor(nullptr, IDC_WAIT));
	auto& nmhdr = *reinterpret_cast<NMLVFINDITEM*>(pnmh);

	std::string text(Str(nmhdr.lvfi.psz).str());
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

LRESULT CLogView::OnBeginDrag(NMHDR* pnmh)
{
	auto& nmhdr = *reinterpret_cast<NMLISTVIEW*>(pnmh);

	LVHITTESTINFO info;
	info.flags = 0;
	info.pt = nmhdr.ptAction;
	SubItemHitTest(&info);
	if ((info.flags & LVHT_ONITEM) == 0 || info.iSubItem != ColumnToSubItem(Column::Message))
	{
		SetMsgHandled(false);
		return 0;
	}

	StopTracking();

	SetCapture();
	m_dragging = true; 
	m_dragStart = nmhdr.ptAction;
	m_dragStart.x += GetScrollPos(SB_HORZ);
	m_dragEnd = nmhdr.ptAction;
	m_dragEnd.x += GetScrollPos(SB_HORZ);

	return 0;
}

void CLogView::ClearView()
{
	m_firstLine = m_logFile.Count();
	Clear();
}

void CLogView::OnViewClear(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	ClearView();
}

void CLogView::OnViewReset(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	ResetToLine(0);
}

void CLogView::OnViewResetToLine(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	int begin = GetNextItem(-1, LVNI_FOCUSED);
	if (begin < 0)
		return;
	ResetToLine(m_logLines[begin].line);
}

void CLogView::ResetToLine(int line)
{
	StopTracking();
	m_firstLine = line;
	ApplyFilters();
}

void CLogView::OnViewExcludeLines(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	auto messages = GetSelectedMessages();
	int size = std::min(size_t(100), messages.size());
	for (int i = 0; i < size; ++i)
	{
		m_filter.messageFilters.push_back(Filter(messages[i], MatchType::Simple, FilterType::Exclude, RGB(255, 255, 255), RGB(0, 0, 0)));
	}
	ApplyFilters();
}

void CLogView::OnViewSelectAll(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	SelectAll();
}

void CLogView::OnViewCopy(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	Copy();
}

void CLogView::OnViewAutoScroll(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	SetAutoScroll(!GetAutoScroll());
}

void CLogView::OnViewAutoScrollStop(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	SetAutoScrollStop(!GetAutoScrollStop());
}

void CLogView::OnViewTime(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	SetClockTime(!GetClockTime());
}

void CLogView::OnViewProcessColors(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	SetViewProcessColors(!GetViewProcessColors());
}

void CLogView::OnEscapeKey(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	SetHighlightText();
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

bool CLogView::FindProcess(int direction)
{
	int begin = GetNextItem(-1, LVNI_FOCUSED);
	if (begin < 0)
		return false;

	auto processName = m_logFile[m_logLines[begin].line].processName;
	int line = FindLine([processName, this](const LogLine& line) { return m_logFile[line.line].processName == processName; }, direction);
	if (line < 0 || line == begin)
		return false;

	StopTracking();
	ScrollToIndex(line, true);
	return true;
}

void CLogView::OnViewNextProcess(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	FindProcess(+1);
}

void CLogView::OnViewPreviousProcess(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	FindProcess(-1);
}

void CLogView::AddProcessFilter(FilterType::type filterType, COLORREF bgColor, COLORREF fgColor)
{
	std::unordered_set<std::string> names;
	int item = -1;
	while ((item = GetNextItem(item, LVNI_ALL | LVNI_SELECTED)) >= 0)
		names.insert(m_logFile[m_logLines[item].line].processName);

	for (auto& name : names)
		m_filter.processFilters.push_back(Filter(Str(name), MatchType::Simple, filterType, bgColor, fgColor));

	ApplyFilters();
}

void CLogView::OnViewProcessHighlight(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	AddProcessFilter(FilterType::Highlight, GetRandomBackColor());
}

void CLogView::OnViewProcessInclude(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	AddProcessFilter(FilterType::Include);
}

void CLogView::OnViewProcessExclude(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	AddProcessFilter(FilterType::Exclude);
}

void CLogView::OnViewProcessTrack(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	AddProcessFilter(FilterType::Track, GetRandomBackColor());
}

void CLogView::OnViewProcessOnce(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	AddProcessFilter(FilterType::Once, GetRandomBackColor());
}

void CLogView::AddMessageFilter(FilterType::type filterType, COLORREF bgColor, COLORREF fgColor)
{
	if (m_highlightText.empty())
		return;

	m_filter.messageFilters.push_back(Filter(Str(m_highlightText), MatchType::Simple, filterType, bgColor, fgColor));
	ApplyFilters();
}

void CLogView::OnViewFilterHighlight(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	AddMessageFilter(FilterType::Highlight, GetRandomBackColor());
}

void CLogView::OnViewFilterInclude(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	AddMessageFilter(FilterType::Include);
}

void CLogView::OnViewFilterExclude(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	AddMessageFilter(FilterType::Exclude);
}

void CLogView::OnViewFilterToken(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	AddMessageFilter(FilterType::Token, Colors::BackGround, GetRandomTextColor());
}

void CLogView::OnViewFilterTrack(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	AddMessageFilter(FilterType::Track, GetRandomBackColor());
}

void CLogView::OnViewFilterOnce(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	AddMessageFilter(FilterType::Once, GetRandomBackColor());
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
	int line = FindLine([](const LogLine& line) { return line.bookmark; }, direction);
	if (line >= 0)
		ScrollToIndex(line, false);
}

void CLogView::OnViewNextBookmark(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	FindBookmark(+1);
}

void CLogView::OnViewPreviousBookmark(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	FindBookmark(-1);
}

void CLogView::OnViewClearBookmarks(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	for (auto& line : m_logLines)
		line.bookmark = false;
	Invalidate();
}

void CLogView::DoPaint(CDCHandle dc)
{
	RECT rect;
	dc.GetClipBox(&rect);
	dc.FillSolidRect(&rect, Colors::BackGround);
 
	DefWindowProc(WM_PAINT, reinterpret_cast<WPARAM>(dc.m_hDC), 0);
}

std::wstring CLogView::GetName() const
{
	return m_name;
}

void CLogView::SetName(const std::wstring& name)
{
	m_name = name;
}

void CLogView::SetFont(HFONT hFont)
{
	CListViewCtrl::SetFont(hFont);
	GetHeader().Invalidate();

	// Trigger WM_MEASUREPOS
	// See: http://www.codeproject.com/Articles/1401/Changing-Row-Height-in-an-owner-drawn-Control
	CRect rect;
	GetWindowRect(&rect);
	WINDOWPOS wp;
	wp.hwnd = *this;
	wp.cx = rect.Width();
	wp.cy = rect.Height();
	wp.flags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER;
	SendMessage(WM_WINDOWPOSCHANGED, 0, reinterpret_cast<LPARAM>(&wp));
}

bool CLogView::GetAutoScroll() const
{
	return m_autoScrollDown;
}

void CLogView::SetAutoScroll(bool enable)
{
	m_autoScrollDown = enable;
	if (enable)
		ScrollDown();
}

bool CLogView::GetAutoScrollStop() const
{
	return m_autoScrollStop;
}

void CLogView::SetAutoScrollStop(bool enable)
{
	m_autoScrollStop = enable;
}

void CLogView::Clear()
{
	SetItemCount(0);
	m_dirty = false;
	m_logLines.clear();
	m_highlightText.clear();
	if (m_autoScrollStop)
		m_autoScrollDown = true;

	ResetFilters();
}

int CLogView::GetFocusLine() const
{
	int item = GetNextItem(-1, LVNI_FOCUSED);
	if (item < 0)
		return -1;

	return m_logLines[item].line;
}

void CLogView::SetFocusLine(int line)
{
	auto it = std::upper_bound(m_logLines.begin(), m_logLines.end(), line, [](int line, const LogLine& logLine) { return line < logLine.line; });
	ScrollToIndex(it - m_logLines.begin() - 1, false);
}

void CLogView::Add(int beginIndex, int line, const Message& msg)
{
	if (IsClearMessage(msg))
		ClearView();

	if (!IsIncluded(msg))
		return;

	if (IsBeepMessage(msg))
		MessageBeep(0xFFFFFFFF);	// A simple beep. If the sound card is not available, the sound is generated using the speaker.

	m_dirty = true;
	m_changed = true;
	auto it = m_logLines.begin();
	while (it != m_logLines.end() && it->line < beginIndex)
		++it;
	m_logLines.erase(m_logLines.begin(), it);

	int viewline = m_logLines.size();
	m_logLines.push_back(LogLine(line));

	if (m_autoScrollDown && MatchFilterType(FilterType::Stop, msg))
	{
		m_stop = [this, viewline] ()
		{
			StopScrolling();
			ScrollToIndex(viewline, true);
		};
		return;
	}

	if (MatchFilterType(FilterType::Track, msg))
	{
		m_autoScrollDown = false;
		m_track = [this, viewline] () 
		{ 
			return ScrollToIndex(viewline, true);
		};
	}
}

void CLogView::BeginUpdate()
{
	m_changed = false;
}

bool CLogView::EndUpdate()
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
		m_stop = nullptr;
	}
	if (m_track) 
	{
		if (m_track())
		{
			// nolonger track item after it has correctly centered
			StopTracking();
		}
	}
	return m_changed;
}

void CLogView::StopTracking()
{
	m_track = nullptr;
}

void CLogView::StopScrolling()
{
	m_autoScrollDown = false;
	for (auto& filter : m_filter.messageFilters)
	{
		if (filter.filterType == FilterType::Track)
		{
			filter.enable = false;
		}
	}
	for (auto& filter : m_filter.processFilters)
	{
		if (filter.filterType == FilterType::Track)
		{
			filter.enable = false;
		}
	}
	StopTracking();
}

void CLogView::ClearSelection()
{
	int item = -1;
	for (;;)
	{
		item = GetNextItem(item, LVNI_SELECTED);
		if (item < 0)
			break;
		SetItemState(item, 0, LVIS_SELECTED);
	}
}

// returns false if centering was requested but not executed
//         because there where not enough lines below the requested index
//		   and it can be usefull to call ScrollToIndex again when more lines are available
bool CLogView::ScrollToIndex(int index, bool center)
{
	if (index < 0 || index >= static_cast<int>(m_logLines.size()))
		return true;

	ClearSelection();
	SetItemState(index, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

	int paddingLines = GetCountPerPage() / 2;
	int minTopIndex = std::max(0, index - paddingLines);

	// this ensures the line is visible and not at the top of the view 
	// if it does not need to be, also when coming from a higher index
	EnsureVisible(minTopIndex, false);
	EnsureVisible(index, false);
	
	if (index > paddingLines)
	{
		// if there are more items above the index then half a page, then centering may be possible.
		if (center)
		{
			int maxBottomIndex = std::min<int>(m_logLines.size() - 1, index + paddingLines);
			EnsureVisible(maxBottomIndex, false);
			return (maxBottomIndex == (index + paddingLines));
		}
	}
	return true;
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

void CLogView::SetViewProcessColors(bool value)
{
	m_processColors = value;
	Invalidate(false);
}

bool CLogView::GetViewProcessColors() const
{
	return m_processColors;
}

void CLogView::SelectAll()
{
	int lines = GetItemCount();
	for (int i = 0; i < lines; ++i)
		SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
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
		GetColumnText(item, Column::Line) << "\t" <<
		GetColumnText(item, Column::Time) << "\t" <<
		GetColumnText(item, Column::Pid) << "\t" <<
		GetColumnText(item, Column::Process) << "\t" <<
		GetColumnText(item, Column::Message);
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
			ss << GetItemText(item) << "\r\n";
	}
	const std::string& str = ss.str();

	Win32::HGlobal hdst(GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, str.size() + 1));
	Win32::GlobalLock<char> lock(hdst);
	std::copy(str.begin(), str.end(), stdext::checked_array_iterator<char*>(lock.Ptr(), str.size()));
	lock.Ptr()[str.size()] = '\0';
	if (OpenClipboard())
	{
		EmptyClipboard();
		SetClipboardData(CF_TEXT, hdst.release());
		CloseClipboard();
	}
}

std::wstring CLogView::GetHighlightText() const
{
	return m_highlightText;
}

void CLogView::SetHighlightText(const std::wstring& text)
{
	if (m_highlightText != text)
	{
		m_highlightText = text;
		Invalidate(false);
	}
}

template <typename Predicate>
int CLogView::FindLine(Predicate pred, int direction) const
{
	SetCursor(::LoadCursor(nullptr, IDC_ARROW));
	Win32::ScopedCursor cursor(::LoadCursor(nullptr, IDC_WAIT));

	int begin = std::max(GetNextItem(-1, LVNI_FOCUSED), 0);
	int line = begin;

	if (m_logLines.empty())
		return -1;

	do
	{
		line += direction;
		if (line < 0)
			line += m_logLines.size();
		if (line >= static_cast<int>(m_logLines.size()))
			line -= m_logLines.size();

		if (pred(m_logLines[line]))
			return line;
	}
	while (line != begin);

	return -1;
}

bool CLogView::Find(const std::string& text, int direction)
{
	StopTracking();

	int line = FindLine([text, this](const LogLine& line) { return Contains(m_logFile[line.line].text, text); }, direction);
	if (line < 0)
		return false;

	bool sameLine = GetItemState(line, LVIS_FOCUSED) != 0;
	if (!sameLine)
		ScrollToIndex(line, true);

	auto wtext = WStr(text).str();
	if (sameLine && wtext == m_highlightText)
		return false;

	SetHighlightText(wtext);
	return true;
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
	SetName(Win32::RegGetStringValue(reg));
	SetAutoScrollStop(Win32::RegGetDWORDValue(reg, L"AutoScrollStop", 1) != 0);
	SetClockTime(Win32::RegGetDWORDValue(reg, L"ClockTime", 1) != 0);
	SetViewProcessColors(Win32::RegGetDWORDValue(reg, L"ShowProcessColors", 0) != 0);

	std::vector<ColumnInfo> columns;
	for (int i = 0; i < Column::Count; ++i)
	{
		CRegKey regColumn;
		if (regColumn.Open(reg, WStr(wstringbuilder() << L"Columns\\Column" << i)) != ERROR_SUCCESS)
			break;

		ColumnInfo column = m_columns[i];
		column.enable = Win32::RegGetDWORDValue(regColumn, L"Enable", column.enable) != 0;
		column.column.cx = Win32::RegGetDWORDValue(regColumn, L"Width", column.column.cx);
		column.column.iOrder = Win32::RegGetDWORDValue(regColumn, L"Order", column.column.iOrder);
		columns.push_back(column);
	}
	if (columns.size() == m_columns.size())
		m_columns.swap(columns);

	CRegKey regFilters;
	if (regFilters.Open(reg, L"MessageFilters") == ERROR_SUCCESS)
		LoadFilterSettings(m_filter.messageFilters, regFilters);
	if (regFilters.Open(reg, L"ProcessFilters") == ERROR_SUCCESS)
		LoadFilterSettings(m_filter.processFilters, regFilters);

	ApplyFilters();
	UpdateColumns();
}

void CLogView::SaveSettings(CRegKey& reg)
{
	UpdateColumnInfo();

	reg.SetDWORDValue(L"AutoScrollStop", GetAutoScrollStop());
	reg.SetDWORDValue(L"ClockTime", GetClockTime());
	reg.SetDWORDValue(L"ShowProcessColors", GetViewProcessColors());

	int i = 0;
	for (auto& col : m_columns)
	{
		CRegKey regColumn;
		regColumn.Create(reg, WStr(wstringbuilder() << L"Columns\\Column" << i));
		regColumn.SetDWORDValue(L"Enable", col.enable);
		regColumn.SetDWORDValue(L"Width", col.column.cx);
		regColumn.SetDWORDValue(L"Order", col.column.iOrder);
		++i;
	}

	CRegKey regFilters;
	if (regFilters.Create(reg, L"MessageFilters") == ERROR_SUCCESS)
		SaveFilterSettings(m_filter.messageFilters, regFilters);
	if (regFilters.Create(reg, L"ProcessFilters") == ERROR_SUCCESS)
		SaveFilterSettings(m_filter.processFilters, regFilters);
}

void CLogView::Save(const std::wstring& filename) const
{
	std::ofstream fs;
	OpenLogFile(fs, filename);

	int lines = GetItemCount();
	for (int i = 0; i < lines; ++i)
	{
		int line = m_logLines[i].line;
		const Message& msg = m_logFile[line];
		WriteLogFileMessage(fs, msg.time, msg.systemTime, msg.processId, msg.processName, msg.text);
	}

	fs.close();
	if (!fs)
		Win32::ThrowLastError(filename);
}

LogFilter CLogView::GetFilters() const
{
	return m_filter;
}

void CLogView::SetFilters(const LogFilter& filter)
{
	StopTracking();
	m_filter = filter;
	ApplyFilters();
}

std::vector<int> CLogView::GetBookmarks() const
{
	std::vector<int> bookmarks;
	for (auto& line : m_logLines)
		if (line.bookmark)
			bookmarks.push_back(line.line);
	return bookmarks;
}

void CLogView::ResetFilters()
{
	for (auto& filter : m_filter.messageFilters)
	{
		if (filter.filterType == FilterType::Once)
			filter.matched = false;
	}
	for (auto& filter : m_filter.processFilters)
	{
		if (filter.filterType == FilterType::Once)
			filter.matched = false;
	}
	m_matchColors.clear();
}

void CLogView::ApplyFilters()
{
	ResetFilters();
	ClearSelection();

	int focusItem = GetNextItem(-1, LVIS_FOCUSED);
	SetItemState(focusItem, 0, LVIS_FOCUSED);
	int focusLine = focusItem < 0 ? -1 : m_logLines[focusItem].line;

	auto bookmarks = GetBookmarks();
	auto itBookmark = bookmarks.begin();

	std::deque<LogLine> logLines;
//	logLines.reserve(m_logLines.size());
	int count = m_logFile.Count();
	int line = m_firstLine;
	int item = 0;
	focusItem = -1;
	while (line < count)
	{
		if (IsIncluded(m_logFile[line]))
		{
			logLines.emplace_back(LogLine(line));
			if (itBookmark != bookmarks.end() && *itBookmark == line)
			{
				logLines.back().bookmark = true;
				++itBookmark;
			}

			if (line <= focusLine)
				focusItem = item;

			++item;
		}
		++line;
	}

	m_logLines.swap(logLines);
	SetItemCountEx(m_logLines.size(), LVSICF_NOSCROLL);
	ScrollToIndex(focusItem, false);
	SetItemState(focusItem, LVIS_FOCUSED, LVIS_FOCUSED);
	EndUpdate();
}

bool FilterSupportsColor(FilterType::type value)
{
	switch (value)
	{
	case FilterType::Include:
	case FilterType::Highlight:
	case FilterType::Track:
	case FilterType::Stop:
	case FilterType::Once:
		return true;
	}
	return false;
}

std::vector<Filter> MoveHighlighFiltersToFront(std::vector<Filter> filters)
{
	std::stable_partition(filters.begin(), filters.end(), [](const Filter& f) { return f.filterType == FilterType::Highlight; });
	return filters;
}

TextColor CLogView::GetTextColor(const Message& msg) const
{
	auto messageFilters = MoveHighlighFiltersToFront(m_filter.messageFilters);
	for (auto& filter : messageFilters)
	{
		std::smatch match;
		if (filter.enable && FilterSupportsColor(filter.filterType) && std::regex_search(msg.text, match, filter.re))
		{
			if (filter.bgColor == Colors::Auto)
			{
				auto it = m_matchColors.find(MatchKey(match, filter.matchType));
				if (it != m_matchColors.end())
					return TextColor(it->second, Colors::Text);
			}
			else
			{
				return TextColor(filter.bgColor, filter.fgColor);
			}
		}
	}

	auto processFilters = MoveHighlighFiltersToFront(m_filter.processFilters);
	for (auto& filter : processFilters)
	{
		if (filter.enable && FilterSupportsColor(filter.filterType) && std::regex_search(msg.processName, filter.re))
			return TextColor(filter.bgColor, filter.fgColor);
	}

	return TextColor(m_processColors ? msg.color : Colors::BackGround, Colors::Text);
}

bool CLogView::IsClearMessage(const Message& msg) const
{
	using debugviewpp::MatchFilterType;
	return MatchFilterType(m_filter.messageFilters, FilterType::Clear, msg.text);
}

bool CLogView::IsBeepMessage(const Message& msg) const
{
	using debugviewpp::MatchFilterType;
	return MatchFilterType(m_filter.messageFilters, FilterType::Beep, msg.text) || MatchFilterType(m_filter.processFilters, FilterType::Beep, msg.text);
}

bool CLogView::IsIncluded(const Message& msg)
{
	using debugviewpp::IsIncluded;
	return IsIncluded(m_filter.processFilters, msg.processName, m_matchColors) && IsIncluded(m_filter.messageFilters, msg.text, m_matchColors);
}

bool CLogView::MatchFilterType(FilterType::type type, const Message& msg) const
{
	using debugviewpp::MatchFilterType;
	return
		MatchFilterType(m_filter.messageFilters, type, msg.text) ||
		MatchFilterType(m_filter.processFilters, type, msg.processName);
}

} // namespace debugviewpp 
} // namespace fusion
