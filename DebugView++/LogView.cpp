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
#include "dbgstream.h"
#include "Win32Lib.h"
#include "Utilities.h"
#include "Resource.h"
#include "MainFrame.h"
#include "LogView.h"

namespace fusion {
namespace debugviewpp {

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

Highlight::Highlight(int begin, int end, const TextColor& color) :
	begin(begin), end(end), color(color)
{
}

LogLine::LogLine(int line) :
	bookmark(false), line(line)
{
}

ItemData::ItemData() :
	color(GetSysColor(COLOR_WINDOW), GetSysColor(COLOR_WINDOWTEXT))
{
}

BEGIN_MSG_MAP_TRY(CLogView)
	MSG_WM_CREATE(OnCreate)
	MSG_WM_CONTEXTMENU(OnContextMenu)
	MSG_WM_LBUTTONDOWN(OnLButtonDown)
	MSG_WM_MOUSEMOVE(OnMouseMove)
	MSG_WM_LBUTTONUP(OnLButtonUp)
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
	COMMAND_ID_HANDLER_EX(ID_VIEW_CLEAR_BOOKMARKS, OnViewClearBookmarks)
	COMMAND_RANGE_HANDLER_EX(ID_VIEW_COLUMN_FIRST, ID_VIEW_COLUMN_LAST, OnViewColumn)
	DEFAULT_REFLECTION_HANDLER()
	CHAIN_MSG_MAP(COffscreenPaint<CLogView>)
END_MSG_MAP_CATCH(ExceptionHandler)

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
	for (auto it = m_columns.begin(); it != m_columns.end(); ++it)
		if (it->column.iSubItem != column.column.iSubItem && it->column.iOrder >= column.column.iOrder)
			it->column.iOrder += delta;

	UpdateColumns();
}

CLogView::CLogView(const std::wstring& name, CMainFrame& mainFrame, LogFile& logFile, LogFilter filter) :
	m_name(name),
	m_mainFrame(mainFrame),
	m_logFile(logFile),
	m_filter(std::move(filter)),
	m_firstLine(0),
	m_clockTime(false),
	m_autoScrollDown(true),
	m_dirty(false),
	m_insidePaint(false),
	m_hBookmarkIcon(static_cast<HICON>(LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_BOOKMARK), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR))),
	m_dragStart(0, 0),
	m_dragEnd(0, 0)
{
}

void CLogView::ExceptionHandler()
{
	MessageBox(WStr(GetExceptionMessage()).c_str(), LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
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
		column.mask = LVCF_WIDTH | LVCF_ORDER;
		GetColumn(i, &column);
	}
}

void CLogView::UpdateColumns()
{
	int columns = GetHeader().GetItemCount();
	for (int i = 0; i < columns; ++i)
		DeleteColumn(0);

	int col = 0;
	for (auto it = m_columns.begin(); it != m_columns.end(); ++it)
	{
		if (it->enable)
			InsertColumn(col++, &it->column);
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

	SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_HEADERDRAGDROP);
	m_hdr.SubclassWindow(GetHeader());

	m_columns.push_back(MakeColumn(Column::Bookmark, L"", LVCFMT_RIGHT, 20));
	m_columns.push_back(MakeColumn(Column::Line, L"Line", LVCFMT_RIGHT, 60));
	m_columns.push_back(MakeColumn(Column::Time, L"Time", LVCFMT_RIGHT, 90));
	m_columns.push_back(MakeColumn(Column::Pid, L"PID", LVCFMT_RIGHT, 60));
	m_columns.push_back(MakeColumn(Column::Process, L"Process", LVCFMT_LEFT, 140));
	m_columns.push_back(MakeColumn(Column::Message, L"Message", LVCFMT_LEFT, 1500));
	UpdateColumns();

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
		menuId = info.iSubItem == 4 ? IDR_PROCESS_CONTEXTMENU : IDR_VIEW_CONTEXTMENU;
	else
		return;

	CMenu menuContext;
	menuContext.LoadMenu(menuId);
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

void CLogView::OnLButtonDown(UINT /*flags*/, CPoint point)
{
	SetMsgHandled(false);

	StopTracking();
	m_dragStart = point;
	m_dragEnd = point;
	SetHighlightText();
}

void CLogView::OnMouseMove(UINT flags, CPoint point)
{
	SetMsgHandled(false);

	if ((flags & MK_LBUTTON) == 0)
		return;

	m_dragEnd = point;
	Invalidate();
}

void CLogView::OnLButtonUp(UINT /*flags*/, CPoint point)
{
	SetMsgHandled(false);

	if (abs(point.x - m_dragStart.x) <= GetSystemMetrics(SM_CXDRAG) &&
		abs(point.y - m_dragStart.y) <= GetSystemMetrics(SM_CYDRAG))
		return;

	LVHITTESTINFO info;
	info.flags = 0;
	info.pt = m_dragStart;
	SubItemHitTest(&info);
	int x1 = std::min(m_dragStart.x, point.x);
	int x2 = std::max(m_dragStart.x, point.x);
	m_dragStart = CPoint();
	m_dragEnd = CPoint();
	Invalidate();
	if ((info.flags & LVHT_ONITEM) == 0 || SubItemToColumn(info.iSubItem) != Column::Message)
		return;

	int begin = GetTextIndex(info.iItem, x1);
	int end = GetTextIndex(info.iItem, x2);
	SetHighlightText(GetItemWText(info.iItem, ColumnToSubItem(Column::Message)).substr(begin, end - begin));
}

int CLogView::GetTextIndex(int iItem, int xPos)
{
	CDCHandle dc(GetDC());
	GdiObjectSelection font(dc, GetFont());
	return GetTextIndex(dc, iItem, xPos);
}

int CLogView::GetTextIndex(CDCHandle dc, int iItem, int xPos) const
{
	auto rect = GetSubItemRect(0, ColumnToSubItem(Column::Message), LVIR_BOUNDS);
	int x0 = rect.left + GetHeader().GetBitmapMargin();

	auto text = GetItemWText(iItem, ColumnToSubItem(Column::Message));
	int index = GetTextOffset(dc, text, xPos - x0);
	if (index < 0)
		return xPos > x0 ? text.size() : 0;
	return index;
}

LRESULT CLogView::OnDblClick(NMHDR* pnmh)
{
	auto& nmhdr = *reinterpret_cast<NMITEMACTIVATE*>(pnmh);

	if (SubItemToColumn(nmhdr.iSubItem) != Column::Message || nmhdr.iItem < 0 || static_cast<size_t>(nmhdr.iItem) >= m_logLines.size())
		return 0;

	int nFit = GetTextIndex(nmhdr.iItem, nmhdr.ptAction.x);
	auto text = GetItemWText(nmhdr.iItem, ColumnToSubItem(Column::Message));

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
	RECT rect;
	CListViewCtrl::GetSubItemRect(iItem, iSubItem, code, &rect);
	if (iSubItem == 0)
		rect.right = rect.left + GetColumnWidth(0);
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

std::vector<Highlight> CLogView::GetHighlights(const std::string& text) const
{
	std::vector<Highlight> highlights;

	for (auto it = m_filter.messageFilters.begin(); it != m_filter.messageFilters.end(); ++it)
	{
		if (!it->enable)
			continue;

		if (it->type != FilterType::Token)
			continue;

		std::sregex_iterator begin(text.begin(), text.end(), it->re), end;
		for (auto tok = begin; tok != end; ++tok)
			InsertHighlight(highlights, Highlight(tok->position(), tok->position() + tok->length(), TextColor(it->bgColor, it->fgColor)));
	}

	return highlights;
}

void DrawHighlightedText(HDC hdc, const RECT& rect, std::wstring text, std::vector<Highlight> highlights, const std::wstring& highlightText, const Highlight& selection)
{
	auto line = boost::make_iterator_range(text);
	for (;;)
	{
		auto match = boost::algorithm::ifind_first(line, highlightText);
		if (match.empty())
			break;

		InsertHighlight(highlights, Highlight(match.begin() - text.begin(), match.end() - text.begin(), TextColor(RGB(255, 255, 55), RGB(0, 0, 0))));
		line = boost::make_iterator_range(match.end(), line.end());
	}
	InsertHighlight(highlights, selection);

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
			ScopedTextColor txtcol(hdc, it->color.fore);
			ScopedBkColor bkcol(hdc, it->color.back);
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

ItemData CLogView::GetItemData(int iItem) const
{
	ItemData data;
	data.text[Column::Line] = GetItemWText(iItem, ColumnToSubItem(Column::Line));
	data.text[Column::Time] = GetItemWText(iItem, ColumnToSubItem(Column::Time));
	data.text[Column::Pid] = GetItemWText(iItem, ColumnToSubItem(Column::Pid));
	data.text[Column::Process] = GetItemWText(iItem, ColumnToSubItem(Column::Process));
	auto text = TabsToSpaces(m_logFile[m_logLines[iItem].line].text);
	data.highlights = GetHighlights(text);
	data.text[Column::Message] = WStr(text).str();
	data.color = GetTextColor(m_logFile[m_logLines[iItem].line].text);
	return data;
}

bool Contains(const RECT& rect, const POINT& pt)
{
	return pt.x >= rect.left && pt.x < rect.right && pt.y >= rect.top && pt.y < rect.bottom;
}

Highlight CLogView::GetSelectionHighlight(CDCHandle dc, int iItem) const
{
	auto rect = GetSubItemRect(iItem, ColumnToSubItem(Column::Message), LVIR_BOUNDS);
	if (!Contains(rect, m_dragStart))
		return Highlight(0, 0, TextColor(0, 0));

	int x1 = std::min(m_dragStart.x, m_dragEnd.x);
	int x2 = std::max(m_dragStart.x, m_dragEnd.x);

	int begin = GetTextIndex(dc, iItem, x1);
	int end = GetTextIndex(dc, iItem, x2);
	return Highlight(begin, end, TextColor(RGB(128, 255, 255), RGB(0, 0, 0)));	
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
		return DrawHighlightedText(dc, rect, text, data.highlights, m_highlightText, GetSelectionHighlight(dc, iItem));

	HDITEM item;
	item.mask = HDI_FORMAT;
	unsigned align = (GetHeader().GetItem(iSubItem, &item)) ? GetTextAlign(item) : HDF_LEFT;
	dc.DrawText(text.c_str(), text.size(), &rect, align | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

void CLogView::DrawItem(CDCHandle dc, int iItem, unsigned /*iItemState*/) const
{
	auto rect = GetItemRect(iItem, LVIR_BOUNDS);
	auto data = GetItemData(iItem);

	bool selected = GetItemState(iItem, LVIS_SELECTED) == LVIS_SELECTED;
	bool focused = GetItemState(iItem, LVIS_FOCUSED) == LVIS_FOCUSED;
	auto bkColor = selected ? GetSysColor(COLOR_HIGHLIGHT) : data.color.back;
	auto txColor = selected ? GetSysColor(COLOR_HIGHLIGHTTEXT) : data.color.fore;

	rect.left += GetColumnWidth(0);
	dc.FillSolidRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, bkColor);

	ScopedBkColor bcol(dc, bkColor);
	ScopedTextColor tcol(dc, txColor);

	int subitemCount = GetHeader().GetItemCount();
	DrawBookmark(dc, iItem);
	for (int i = 1; i < subitemCount; ++i)
		DrawSubItem(dc, iItem, i, data);
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
//		return CDRF_DODEFAULT;  // Enable this line for non-custom drawing
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT:
		DrawItem(pCustomDraw->nmcd.hdc, pCustomDraw->nmcd.dwItemSpec, pCustomDraw->nmcd.uItemState);
		return CDRF_SKIPDEFAULT;
	}

	return CDRF_DODEFAULT;
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

std::string CLogView::GetColumnText(int iItem, Column::type column) const
{
	int line = m_logLines[iItem].line;
	const Message& msg = m_logFile[line];

	switch (column)
	{
	case Column::Line: return std::to_string(iItem + 1ULL);
	case Column::Time: return m_clockTime ? GetTimeText(msg.systemTime) : GetTimeText(msg.time);
	case Column::Pid: return std::to_string(msg.processId + 0ULL);
	case Column::Process: return msg.processName;
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

	CopyItemText(GetColumnText(item.iItem, SubItemToColumn(item.iSubItem)), item.pszText, item.cchTextMax);
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

void CLogView::OnViewClear(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	m_firstLine = m_logFile.Count();
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

	// Internal Compiler Error on VC2010:
//	int line = FindLine([processName, this](const LogLine& line) { return boost::iequals(m_logFile[line.line].processName, processName); }, direction);
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

void CLogView::OnViewExclude(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	int item = GetNextItem(-1, LVIS_FOCUSED);
	if (item < 0)
		return;

	const auto& name = m_logFile[m_logLines[item].line].processName;
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
			ScrollToIndex(line, false);
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

void CLogView::OnViewClearBookmarks(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	for (auto it = m_logLines.begin(); it != m_logLines.end(); ++it)
		it->bookmark = false;
	Invalidate();
}

void CLogView::DoPaint(CDCHandle dc, const RECT& rcClip)
{
	m_insidePaint = true;

	dc.FillSolidRect(&rcClip, GetSysColor(COLOR_WINDOW));
 
	DefWindowProc(WM_PAINT, reinterpret_cast<WPARAM>(dc.m_hDC), 0);
	m_insidePaint = false;
}

std::wstring CLogView::GetName() const
{
	return m_name;
}

void CLogView::SetName(const std::wstring& name)
{
	m_name = name;
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

void CLogView::Add(int line, const Message& msg)
{
	if (!IsIncluded(msg))
		return;

	m_dirty = true;
	++m_addedLines;
	int viewline = m_logLines.size();

	if (!IsIgnore(msg.text))
		m_scrolldownIndex = viewline;

	m_logLines.push_back(LogLine(line));
	if (m_autoScrollDown && IsStop(msg.text))
	{
		m_stop = [this, viewline] ()
		{
			StopScrolling();
			ScrollToIndex(viewline, true);
		};
		return;
	}

	if (IsTrack(msg.text))
	{
		printf("found: trackitem at line: %d, %s\n", viewline+1, msg.text);
		m_autoScrollDown = false;
		m_track = [this, viewline] () 
		{ 
			return ScrollToIndex(viewline, true);
		};
	}
}

void CLogView::BeginUpdate()
{
//	SetRedraw(false);
	m_addedLines = 0;
}

int CLogView::EndUpdate()
{
//	SetRedraw(true);

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
		if (m_track())
		{
			// nolonger track item after it has correctly centered
			StopTracking();
		}
	}
	return m_addedLines;
}

void CLogView::StopTracking()
{
	m_track = 0;
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
	ScrollToIndex(m_scrolldownIndex, false);
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

std::wstring CLogView::GetHighlightText() const
{
	return m_highlightText;
}

void CLogView::SetHighlightText(const std::wstring& text)
{
	m_highlightText = text;
	Invalidate(false);
}

template <typename Predicate>
int CLogView::FindLine(Predicate pred, int direction) const
{
	ScopedCursor cursor(::LoadCursor(nullptr, IDC_WAIT));

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
	SetClockTime(RegGetDWORDValue(reg, L"ClockTime", 1) != 0);

	for (int i = 0; i < Column::Count; ++i)
	{
		CRegKey regColumn;
		if (regColumn.Open(reg, WStr(wstringbuilder() << L"Columns\\Column" << i)) != ERROR_SUCCESS)
			break;

		auto& column = m_columns[i];
		column.enable = RegGetDWORDValue(regColumn, L"Enable", column.enable) != 0;
		column.column.cx = RegGetDWORDValue(regColumn, L"Width", column.column.cx);
		column.column.iOrder = RegGetDWORDValue(regColumn, L"Order", column.column.iOrder);
	}

	for (int i = 0; ; ++i)
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
	UpdateColumns();
}

void CLogView::SaveSettings(CRegKey& reg)
{
	UpdateColumnInfo();

	reg.SetDWORDValue(L"ClockTime", GetClockTime());

	int i = 0;
	for (auto it = m_columns.begin(); it != m_columns.end(); ++it, ++i)
	{
		CRegKey regFilter;
		regFilter.Create(reg, WStr(wstringbuilder() << L"Columns\\Column" << i));
		regFilter.SetDWORDValue(L"Enable", it->enable);
		regFilter.SetDWORDValue(L"Width", it->column.cx);
		regFilter.SetDWORDValue(L"Order", it->column.iOrder);
	}

	i = 0;
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
	StopTracking();
	m_filter = filter;
	ApplyFilters();
}

std::vector<int> CLogView::GetBookmarks() const
{
	std::vector<int> bookmarks;
	for (auto it = m_logLines.begin(); it != m_logLines.end(); ++it)
		if (it->bookmark)
			bookmarks.push_back(it->line);
	return bookmarks;
}

void CLogView::ApplyFilters()
{
	ClearSelection();

	int focusItem = GetNextItem(-1, LVIS_FOCUSED);
	SetItemState(focusItem, 0, LVIS_FOCUSED);
	int focusLine = focusItem < 0 ? -1 : m_logLines[focusItem].line;

	auto bookmarks = GetBookmarks();
	auto itBookmark = bookmarks.begin();

	std::vector<LogLine> logLines;
	logLines.reserve(m_logLines.size());
	int count = m_logFile.Count();
	int line = m_firstLine;
	int item = 0;
	focusItem = -1;
	while (line < count)
	{
		if (IsIncluded(m_logFile[line]))
		{
			logLines.push_back(LogLine(line));
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
	case FilterType::Ignore:
		return true;
	}
	return false;
}

TextColor CLogView::GetTextColor(const std::string& text) const
{
	for (auto it = m_filter.messageFilters.begin(); it != m_filter.messageFilters.end(); ++it)
	{
		if (it->enable && FilterSupportsColor(it->type) && std::regex_search(text, it->re))
			return TextColor(it->bgColor, it->fgColor);
	}

	for (auto it = m_filter.processFilters.begin(); it != m_filter.processFilters.end(); ++it)
	{
		if (it->enable && FilterSupportsColor(it->type) && std::regex_search(text, it->re))
			return TextColor(it->bgColor, it->fgColor);
	}

	return TextColor(GetSysColor(COLOR_WINDOW), GetSysColor(COLOR_WINDOWTEXT));
}

bool CLogView::IsProcessIncluded(const std::string& process) const
{
	for (auto it = m_filter.processFilters.begin(); it != m_filter.processFilters.end(); ++it)
	{
		if (!it->enable)
			continue;

		switch (it->type)
		{
		case FilterType::Include:
		case FilterType::Exclude:
			if (std::regex_search(process, it->re))
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
	if (!IsProcessIncluded(msg.processName))
		return false;

	return IsMessageIncluded(msg.text);
}

bool CLogView::IsStop(const std::string& text) const
{
	for (auto it = m_filter.messageFilters.begin(); it != m_filter.messageFilters.end(); ++it)
	{
		if (!it->enable)
			continue;

		if (it->type == FilterType::Stop && std::regex_search(text, it->re))
			return true;
	}
	return false;
}

bool CLogView::IsTrack(const std::string& text) const
{
	for (auto it = m_filter.messageFilters.begin(); it != m_filter.messageFilters.end(); ++it)
	{
		if (!it->enable)
			continue;

		if (it->type == FilterType::Track && std::regex_search(text, it->re))
			return true;
	}
	return false;
}

bool CLogView::IsIgnore(const std::string& text) const
{
	for (auto it = m_filter.messageFilters.begin(); it != m_filter.messageFilters.end(); ++it)
	{
		if (!it->enable)
			continue;

		if (it->type == FilterType::Ignore && std::regex_search(text, it->re))
			return true;
	}
	return false;
}

} // namespace debugviewpp 
} // namespace fusion
