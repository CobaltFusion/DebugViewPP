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
#include <boost/property_tree/ptree.hpp>
#include <utility>
#include "CobaltFusion/AtlWinExt.h"
#include "CobaltFusion/stringbuilder.h"
#include "CobaltFusion/dbgstream.h"
#include "CobaltFusion/fusionassert.h"
#include "Win32/Registry.h"
#include "DebugView++Lib/Conversions.h"
#include "DebugView++Lib/FileIO.h"
#include "resource.h"
#include "MainFrame.h"
#include "LogView.h"
#include "RenameProcessDlg.h"
//#include "VersionHelpers.h"  // IsWindows10OrGreater ??

namespace fusion {
namespace debugviewpp {

unsigned GetTextAlign(const HDITEM& item)
{
    switch (item.fmt & HDF_JUSTIFYMASK)
    {
    case HDF_LEFT: return DT_LEFT;
    case HDF_CENTER: return DT_CENTER;
    case HDF_RIGHT: return DT_RIGHT;
    default: break;
    }
    return HDF_LEFT;
}

SIZE GetTextSize(CDCHandle dc, const std::wstring& text, int length)
{
    SIZE size = {0};
    dc.GetTextExtent(text.c_str(), length, &size);
    return size;
}

void ExtTextOut(HDC hdc, const POINT& pt, const RECT& rect, const std::wstring& text)
{
    ::ExtTextOutW(hdc, pt.x, pt.y, ETO_CLIPPED | ETO_OPAQUE, &rect, text.c_str(), static_cast<UINT>(text.size()), nullptr);
}

int GetTextOffset(HDC hdc, const std::string& s, int xPos)
{
    auto exp = TabsToSpaces(s);
    int nFit = 0;
    SIZE size = {};
    if (GetTextExtentExPointA(hdc, exp.c_str(), static_cast<int>(exp.size()), xPos, &nFit, nullptr, &size) == 0)
    {
        return 0;
    }
    return SkipTabOffset(s, nFit);
}

int GetTextOffset(HDC hdc, const std::wstring& s, int xPos)
{
    auto exp = TabsToSpaces(s);
    int nFit = 0;
    SIZE size = {};
    if (xPos <= 0 || (GetTextExtentExPointW(hdc, exp.c_str(), static_cast<int>(exp.size()), xPos, &nFit, nullptr, &size) == 0))
    {
        return 0;
    }
    return SkipTabOffset(s, nFit);
}

void AddEllipsis(HDC hdc, std::wstring& text, int width)
{
    static const std::wstring ellipsis(L"...");
    int pos = GetTextOffset(hdc, text, width);
    if (pos >= 0 && pos < static_cast<int>(text.size()))
    {
        pos = GetTextOffset(hdc, text, width - GetTextSize(hdc, ellipsis, static_cast<int>(ellipsis.size())).cx);
        text = text.substr(0, pos) + ellipsis;
    }
}

SelectionInfo::SelectionInfo()

{
}

SelectionInfo::SelectionInfo(int beginLine, int endLine, int count) :
    beginLine(beginLine),
    endLine(endLine),
    count(count)
{
}

TextColor::TextColor(COLORREF back, COLORREF fore) :
    back(back),
    fore(fore)
{
}

Highlight::Highlight(int id, int begin, int end, const TextColor& color) :
    id(id),
    begin(begin),
    end(end),
    color(color)
{
}

LogLine::LogLine(int line) :
    bookmark(false),
    line(line)
{
}

ItemData::ItemData() :
    color(Colors::BackGround, Colors::Text)
{
}

BEGIN_MSG_MAP2(CLogView)
    MSG_WM_CREATE(OnCreate)
    // OnClose messages are sent only to top-level windows
    MSG_WM_CONTEXTMENU(OnContextMenu)
    MSG_WM_SETCURSOR(OnSetCursor)
    MSG_WM_LBUTTONDOWN(OnLButtonDown)
    MSG_WM_MOUSEMOVE(OnMouseMove)
    MSG_WM_LBUTTONUP(OnLButtonUp)
    MSG_WM_TIMER(OnTimer)
    MSG_WM_KEYDOWN(OnKeyDown)
    REFLECTED_NOTIFY_CODE_HANDLER_EX(NM_CLICK, OnClick)
    REFLECTED_NOTIFY_CODE_HANDLER_EX(NM_DBLCLK, OnDblClick)
    REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_ITEMCHANGED, OnItemChanged)
    REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_GETDISPINFO, OnGetDispInfo)
    REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_INCREMENTALSEARCH, OnIncrementalSearch)
    REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_ODCACHEHINT, OnOdCacheHint)
    REFLECTED_NOTIFY_CODE_HANDLER_EX(LVN_BEGINDRAG, OnBeginDrag)
    REFLECTED_NOTIFY_CODE_HANDLER_EX(NM_CUSTOMDRAW, OnCustomDraw)
    COMMAND_ID_HANDLER_EX(ID_VIEW_CLEAR, OnViewClear)
    COMMAND_ID_HANDLER_EX(ID_VIEW_EXCLUDE_LINES, OnViewExcludeLines)
    COMMAND_ID_HANDLER_EX(ID_VIEW_RESET, OnViewReset)
    COMMAND_ID_HANDLER_EX(ID_VIEW_RESET_TO_LINE, OnViewResetToLine)
    COMMAND_ID_HANDLER_EX(ID_VIEW_SELECTALL, OnViewSelectAll)
    COMMAND_ID_HANDLER_EX(ID_VIEW_COPY, OnViewCopy)
    COMMAND_ID_HANDLER_EX(ID_VIEW_COPY_MESSAGES, OnViewCopyMessages)
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
    COMMAND_ID_HANDLER_EX(ID_VIEW_PROCESS_RENAME, OnViewProcessRename)
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
    CHAIN_MSG_MAP(CDoubleBufferImpl<CLogView>) //DrMemory: GDI USAGE ERROR: DC 0x3e011cca that contains selected object being deleted

    // Provides a default handler that will receive reflected messages; the handler will properly pass unhandled messages to DefWindowProc.
    DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()

bool CLogView::IsColumnViewed(int nID) const
{
    return m_columns[nID - ID_VIEW_COLUMN_FIRST].enable;
}

void CLogView::OnKeyDown(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
    bool shift = GetKeyState(VK_SHIFT) < 0;
    bool ctrl = GetKeyState(VK_CONTROL) < 0;
    if (nChar == 'C')
    {
        if (shift && ctrl)
        {
            CopyMessagesToClipboard();
        }
        else if (ctrl)
        {
            Copy();
        }
    }
    else
    {
        SetMsgHandled(win32::False);
    }
}

void CLogView::OnViewColumn(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
    auto& column = m_columns[nID - ID_VIEW_COLUMN_FIRST];
    column.enable = !column.enable;
    UpdateColumns();
}

CLogView::CLogView(std::wstring name, CMainFrame& mainFrame, LogFile& logFile, LogFilter filter) :
    m_name(std::move(name)),
    m_mainFrame(mainFrame),
    m_logFile(logFile),
    m_filter(std::move(filter)),
    m_firstLine(0),
    m_clockTime(false),
    m_processColors(false),
    m_autoScrollDown(true),
    m_autoScrollStop(true),
    m_dirty(false),
    m_changed(false),
    m_hBookmarkIcon(static_cast<HICON>(LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_BOOKMARK), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR))),
    m_hBeamCursor(LoadCursor(nullptr, IDC_IBEAM)),
    m_dragStart(0, 0),
    m_dragEnd(0, 0),
    m_dragging(false),
    m_scrollX(0)
{
}

void CLogView::OnException() const
{
    FUSION_REPORT_EXCEPTION("Unknown Exception");
}

void CLogView::OnException(const std::exception& ex)
{
    FUSION_REPORT_EXCEPTION(ex.what());
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
        {
            return iSubItem;
        }
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

void CLogView::UpdateColumns()
{
    int columns = GetHeader().GetItemCount();
    for (int i = 0; i < columns; ++i)
    {
        DeleteColumn(0);
    }

    int col = 0;
    for (auto& item : m_columns)
    {
        if (item.enable)
        {
            InsertColumn(col++, &item.column);
        }
    }
}

ColumnInfo MakeColumn(Column::type column, const wchar_t* name, int format, int width)
{
    auto info = ColumnInfo();
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

    m_pDropTargetSupport = Win32::CreateComObject<DropTargetSupport>();
    m_pDropTargetSupport->Register(*this);
    m_pDropTargetSupport->SubscribeToDropped([this](const std::wstring& uri) {
        m_mainFrame.OnDropped(uri);
    });
    return 0;
}

CLogView::~CLogView()
{
    m_pDropTargetSupport->Unregister();
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
        SetMsgHandled(win32::False);
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

    SetMsgHandled(win32::False);
    return FALSE;
}

int CLogView::TextHighlightHitTest(int iItem, const POINT& pt)
{
    int pos = GetTextIndex(iItem, pt.x);
    auto highlights = GetItemData(iItem).highlights;
    auto it = highlights.begin();
    while (it != highlights.end() && it->end <= pos)
    {
        ++it;
    }
    if (it != highlights.end() && it->begin <= pos)
    {
        return it->id;
    }
    return 0;
}

std::vector<std::string> CLogView::GetSelectedMessages() const
{
    std::vector<std::string> messages;
    int item = -1;
    while ((item = GetNextItem(item, LVNI_ALL | LVNI_SELECTED)) >= 0)
    {
        messages.push_back(Str(GetColumnText(item, Column::Message)));
    }
    return messages;
}

void CLogView::OnContextMenu(HWND /*hWnd*/, CPoint pt)
{
    if ((pt == CPoint(-1, -1)) != 0)
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
    {
        menuId = IDR_HEADER_CONTEXTMENU;
    }
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
    {
        return;
    }

    CMenu menuContext;
    menuContext.LoadMenu(menuId);
    CMenuHandle menuPopup(menuContext.GetSubMenu(0));
    ClientToScreen(&pt);
    menuPopup.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_mainFrame);
}

bool iswordchar(wint_t c)
{
    return (iswalnum(c) != 0) || c == L'_';
}

LRESULT CLogView::OnClick(NMHDR* pnmh)
{
    auto& nmhdr = *reinterpret_cast<NMITEMACTIVATE*>(pnmh);

    LVHITTESTINFO info;
    info.flags = 0;
    info.pt = nmhdr.ptAction;
    SubItemHitTest(&info);
    if ((info.flags & LVHT_ONITEM) != 0 && info.iSubItem == ColumnToSubItem(Column::Bookmark))
    {
        ToggleBookmark(info.iItem);
    }

    return 0;
}

void CLogView::OnLButtonDown(UINT flags, CPoint point)
{
    if ((flags & MK_SHIFT) == 0 || m_highlightText.empty())
    {
        SetMsgHandled(win32::False);
        return;
    }

    LVHITTESTINFO info;
    info.flags = 0;
    info.pt = point;
    SubItemHitTest(&info);
    if ((info.flags & LVHT_ONITEM) == 0 || info.iSubItem != ColumnToSubItem(Column::Message))
    {
        SetMsgHandled(win32::False);
        return;
    }

    CClientDC dc(*this);
    Win32::GdiObjectSelection font(dc, GetFont());

    int x0 = GetSubItemRect(info.iItem, info.iSubItem, LVIR_BOUNDS).left + GetHeader().GetBitmapMargin();
    auto line = TabsToSpaces(GetItemWText(info.iItem, ColumnToSubItem(Column::Message)));
    int pos = 0;
    int min = 1000 * 1000;
    bool found = false;
    int highlightTextSize = static_cast<int>(m_highlightText.size());
    for (;;)
    {
        pos = static_cast<int>(line.find(m_highlightText, pos));
        if (pos == std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>>::npos)
        {
            break;
        }

        int x1 = x0 + GetTextSize(dc.m_hDC, line, pos).cx;
        int x2 = x0 + GetTextSize(dc.m_hDC, line, pos + highlightTextSize).cx;
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

        pos += highlightTextSize;
        found = true;
    }
    if (!found)
    {
        SetMsgHandled(win32::False);
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
    SetMsgHandled(win32::False);
    if (!m_dragging)
    {
        return;
    }

    m_dragEnd = point;
    m_dragEnd.x += GetScrollPos(SB_HORZ);
    Invalidate();

    RECT rect;
    GetClientRect(&rect);
    if (point.x < rect.left + 32)
    {
        if (m_scrollX == 0)
        {
            SetTimer(1, 25, nullptr);
        }
        m_scrollX = -8;
    }
    else if (point.x > rect.right - 32)
    {
        if (m_scrollX == 0)
        {
            SetTimer(1, 25, nullptr);
        }
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
    SetMsgHandled(win32::False);

    if (m_scrollX != 0)
    {
        KillTimer(1);
    }

    if (!m_dragging)
    {
        return;
    }

    m_dragging = false;

    auto dragStart = m_dragStart;
    dragStart.x -= GetScrollPos(SB_HORZ);

    if (abs(point.x - dragStart.x) <= GetSystemMetrics(SM_CXDRAG) &&
        abs(point.y - dragStart.y) <= GetSystemMetrics(SM_CYDRAG))
    {
        return;
    }

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
    {
        return;
    }

    int begin = GetTextIndex(info.iItem, x1);
    int end = GetTextIndex(info.iItem, x2);
    SetHighlightText(TabsToSpaces(GetItemWText(info.iItem, ColumnToSubItem(Column::Message))).substr(begin, end - begin));
}

void CLogView::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent != 1)
    {
        return;
    }

    Scroll(CSize(m_scrollX, 0));
}

void CLogView::MeasureItem(MEASUREITEMSTRUCT* pMeasureItemStruct) const
{
    CClientDC dc(*this);

    Win32::GdiObjectSelection font(dc, GetFont());
    TEXTMETRIC metric;
    dc.GetTextMetrics(&metric);
    pMeasureItemStruct->itemHeight = metric.tmHeight;
}

void CLogView::DrawItem(DRAWITEMSTRUCT* pDrawItemStruct) const
{
    DrawItem(pDrawItemStruct->hDC, pDrawItemStruct->itemID, pDrawItemStruct->itemState);
}

void CLogView::DeleteItem(DELETEITEMSTRUCT* lParam)
{
    COwnerDraw<CLogView>::DeleteItem(lParam);
}

int CLogView::GetTextIndex(int iItem, int xPos) const
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
    {
        return 0;
    }

    int nFit = GetTextIndex(nmhdr.iItem, nmhdr.ptAction.x);
    auto text = TabsToSpaces(GetItemWText(nmhdr.iItem, ColumnToSubItem(Column::Message)));

    int begin = nFit;
    while (begin > 0)
    {
        if (!iswordchar(text[begin - 1]))
        {
            break;
        }
        --begin;
    }
    int end = nFit;
    while (end < static_cast<int>(text.size()))
    {
        if (!iswordchar(text[end]))
        {
            break;
        }
        ++end;
    }
    SetHighlightText(std::wstring(text.begin() + begin, text.begin() + end));
    return 0;
}

LRESULT CLogView::OnItemChanged(NMHDR* pnmh)
{
    auto& nmhdr = *reinterpret_cast<NMLISTVIEW*>(pnmh);

    if ((nmhdr.uNewState & LVIS_FOCUSED) == 0 ||
        nmhdr.iItem < 0 ||
        static_cast<size_t>(nmhdr.iItem) >= m_logLines.size())
    {
        return 0;
    }

    if (m_autoScrollStop)
    {
        m_autoScrollDown = nmhdr.iItem == GetItemCount() - 1;
    }

    //this breaks F3/ShiftF3 //  SetHighlightText(L"");
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
    {
        rect.right = rect.left + GetColumnWidth(0);
    }
    return rect;
}

void InsertHighlight(std::vector<Highlight>& highlights, const Highlight& highlight)
{
    // if nothing is selected, this still gets called, so ignore the call in that case
    if (highlight.begin == highlight.end)
    {
        return;
    }

    // create a new vector that is two larger
    std::vector<Highlight> newHighlights;
    newHighlights.reserve(highlights.size() + 2);

    // add the highlight between existing, possibly partially overlapping highlights
    auto it = highlights.begin();
    while (it != highlights.end() && it->begin < highlight.begin)
    {
        newHighlights.push_back(*it);
        ++it;
    }

    while (it != highlights.end() && it->end <= highlight.end)
    {
        ++it;
    }

    newHighlights.push_back(highlight);

    while (it != highlights.end())
    {
        newHighlights.push_back(*it);
        ++it;
    }

    highlights.swap(newHighlights);
}

void InsertHighlight(std::vector<Highlight>& highlights, std::wstring_view text, std::wstring match, TextColor color)
{
    auto line = boost::make_iterator_range(text);
    for (;;)
    {
        auto range = boost::algorithm::ifind_first(line, match);
        if (range.empty())
        {
            break;
        }
        int begin = ExpandedTabOffset(text, static_cast<int>(range.begin() - text.begin()));
        int end = ExpandedTabOffset(text, static_cast<int>(range.end() - text.begin()));
        InsertHighlight(highlights, Highlight(1, begin, end, color));
        line = boost::make_iterator_range(range.end(), line.end());
    }
}

std::vector<Highlight> CLogView::GetHighlights(std::wstring_view text) const
{
    std::vector<Highlight> highlights;

    int highlightId = 1;
    for (auto& filter : m_filter.messageFilters)
    {
        if (!filter.enable || filter.filterType != FilterType::Token)
        {
            continue;
        }

        const wchar_t* testEnd = text.data() + text.size();
        std::wstring pattern = WStr(MakePattern(filter.matchType, filter.text));
        std::wregex re(pattern);

        std::wcregex_iterator begin(text.data(), testEnd, re);
        std::wcregex_iterator end;
        int id = ++highlightId;
        for (auto tok = begin; tok != end; ++tok)
        {
            int first = 0;
            int count = 1;
            if (tok->size() > 1 && filter.matchType == MatchType::RegexGroups)
            {
                first = 1;
                count = static_cast<int>(tok->size());
            }
            for (int i = first; i < count; ++i)
            {
                int beginIndex = ExpandedTabOffset(text, static_cast<int>(tok->position(i)));
                int endIndex = ExpandedTabOffset(text, static_cast<int>(tok->position(i) + tok->length(i)));

                if (filter.bgColor == Colors::Auto)
                {
                    auto itc = m_matchColors.find(Str(tok->str(i)));
                    if (itc != m_matchColors.end())
                    {
                        InsertHighlight(highlights, Highlight(id, beginIndex, endIndex, TextColor(itc->second, Colors::Text)));
                    }
                }
                else
                {
                    InsertHighlight(highlights, Highlight(id, beginIndex, endIndex, TextColor(filter.bgColor, filter.fgColor)));
                }
            }
        }
    }

    InsertHighlight(highlights, text, m_highlightText, TextColor(Colors::Highlight, Colors::Text));

    return highlights;
}

void DrawHighlightedText(HDC hdc, const RECT& rect, std::wstring text, std::vector<Highlight> highlights, const Highlight& selection)
{
    InsertHighlight(highlights, selection);
    AddEllipsis(hdc, text, rect.right - rect.left);

    int textSize = static_cast<int>(text.size());
    int height = GetTextSize(hdc, text, textSize).cy;
    POINT pos = {rect.left, rect.top + (rect.bottom - rect.top - height) / 2};
    RECT rcHighlight = rect;
    for (auto& highlight : highlights)
    {
        if (textSize < highlight.end)
        {
            continue;
        }

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
    {
        return;
    }
    RECT rect = GetSubItemRect(iItem, 0, LVIR_BOUNDS);
    dc.DrawIconEx(rect.left /* + GetHeader().GetBitmapMargin() */, rect.top + (rect.bottom - rect.top - 16) / 2, m_hBookmarkIcon.get(), 0, 0, 0, nullptr, DI_NORMAL | DI_COMPAT);
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
    data.highlights = GetHighlights(WStr(m_logFile[m_logLines[iItem].line].text).str());
    data.text[Column::Message] = WStr(text).str();
    data.color = GetTextColor(m_logFile[m_logLines[iItem].line]);
    return data;
}

Highlight CLogView::GetSelectionHighlight(CDCHandle dc, int iItem) const
{
    auto rect = GetSubItemRect(iItem, ColumnToSubItem(Column::Message), LVIR_BOUNDS);
    auto dragStart = m_dragStart;
    dragStart.x -= GetScrollPos(SB_HORZ);
    auto dragEnd = m_dragEnd;
    dragEnd.x -= GetScrollPos(SB_HORZ);
    if (!m_dragging || !Contains(rect, dragStart))
    {
        return Highlight(0, 0, 0, TextColor(0, 0));
    }

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
    {
        return DrawHighlightedText(dc, rect, text, data.highlights, GetSelectionHighlight(dc, iItem));
    }

    HDITEM item;
    item.mask = HDI_FORMAT;
    unsigned align = (GetHeader().GetItem(iSubItem, &item)) != 0 ? GetTextAlign(item) : DT_LEFT;
    dc.DrawText(text.c_str(), static_cast<int>(text.size()), &rect, align | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
}

void CLogView::DrawItem(CDCHandle dc, int iItem, unsigned /*iItemState*/) const
{
    auto rect = GetItemRect(iItem, LVIR_BOUNDS);
    auto data = GetItemData(iItem);

    bool selected = GetItemState(iItem, LVIS_SELECTED) == LVIS_SELECTED;
    //bool focused = GetItemState(iItem, LVIS_FOCUSED) == LVIS_FOCUSED;
    auto bkColor = selected ? Colors::ItemHighlight : data.color.back;
    auto txColor = selected ? Colors::ItemHighlightText : data.color.fore;

    rect.left += GetColumnWidth(0);
    dc.FillSolidRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, bkColor);

    Win32::ScopedBkColor bcol(dc, bkColor);
    Win32::ScopedTextColor tcol(dc, txColor);

    int subitemCount = GetHeader().GetItemCount();
    DrawBookmark(dc, iItem);
    for (int i = 1; i < subitemCount; ++i)
    {
        DrawSubItem(dc, iItem, i, data);
    }
    //if (focused)
    //    dc.DrawFocusRect(&rect);
}

std::wstring CLogView::GetColumnText(int iItem, Column::type column) const
{
    int line = m_logLines[iItem].line;
    const Message& msg = m_logFile[line];

    switch (column)
    {
    case Column::Line: return std::to_wstring(iItem + 1ULL);
    case Column::Date: return WStr(GetDateText(msg.systemTime));
    case Column::Time: return WStr(m_clockTime ? GetTimeText(msg.systemTime) : GetTimeText(msg.time));
    case Column::Pid: return std::to_wstring(msg.processId + 0ULL);
    case Column::Process: return WStr(msg.processName);
    case Column::Message: return WStr(msg.text);
    default: break;
    }
    return L"";
}

LRESULT CLogView::OnGetDispInfo(NMHDR* pnmh)
{
    auto pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pnmh);
    LVITEM& item = pDispInfo->item;
    if ((item.mask & LVIF_TEXT) == 0 || item.iItem >= static_cast<int>(m_logLines.size()))
    {
        return 0;
    }

    m_dispInfoText = GetColumnText(item.iItem, SubItemToColumn(item.iSubItem));
    item.pszText = &m_dispInfoText[0];
    return 0;
}

SelectionInfo CLogView::GetSelectedRange() const
{
    int first = GetNextItem(-1, LVNI_SELECTED);
    if (first < 0)
    {
        return SelectionInfo();
    }

    int item = first;
    int last = first;
    do
    {
        last = item;
        item = GetNextItem(item, LVNI_SELECTED);
    } while (item > 0);

    return SelectionInfo(m_logLines[first].line, m_logLines[last].line, last - first + 1);
}

SelectionInfo CLogView::GetViewRange() const
{
    if (m_logLines.empty())
    {
        return SelectionInfo();
    }

    return SelectionInfo(m_logLines.front().line, m_logLines.back().line, static_cast<int>(m_logLines.size()));
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
    //    int line = nmhdr.iStart; // Does not work as specified...
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
    [[maybe_unused]] auto& nmhdr = *reinterpret_cast<NMLVCACHEHINT*>(pnmh);
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
        SetMsgHandled(win32::False);
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

void CLogView::OnViewClear(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    Clear();
}

void CLogView::OnViewReset(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    ResetToLine(0);
}

void CLogView::OnViewResetToLine(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    int begin = GetNextItem(-1, LVNI_FOCUSED);
    if (begin < 0)
    {
        return;
    }
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
    int size = std::min(100, static_cast<int>(messages.size()));
    for (int i = 0; i < size; ++i)
    {
        m_filter.messageFilters.emplace_back(messages[i], MatchType::Simple, FilterType::Exclude, RGB(255, 255, 255), RGB(0, 0, 0));
    }
    ApplyFilters();
}

void CLogView::OnViewSelectAll(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    SelectAll();
}

void CLogView::OnViewCopyMessages(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CopyMessagesToClipboard();
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
    SetHighlightText(L"");
    StopScrolling();
}

void CLogView::OnViewFindNext(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    if (!m_highlightText.empty())
    {
        FindNext(m_highlightText);
    }
}

void CLogView::OnViewFindPrevious(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    if (!m_highlightText.empty())
    {
        FindPrevious(m_highlightText);
    }
}

bool CLogView::FindProcess(int direction)
{
    int begin = GetNextItem(-1, LVNI_FOCUSED);
    if (begin < 0)
    {
        return false;
    }

    auto processName = m_logFile[m_logLines[begin].line].processName;
    int line = FindLine([processName, this](const LogLine& line) { return m_logFile[line.line].processName == processName; }, direction);
    if (line < 0 || line == begin)
    {
        return false;
    }

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
    {
        names.insert(m_logFile[m_logLines[item].line].processName);
    }

    for (auto& name : names)
    {
        m_filter.processFilters.emplace_back(Str(name), MatchType::Simple, filterType, bgColor, fgColor);
    }

    ApplyFilters();
}

void CLogView::OnViewProcessRename(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    int item = GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
    if (item >= 0)
    {
        auto name = m_logFile[m_logLines[item].line].processName;
        std::wstring wname = WStr(name);
        CRenameProcessDlg dlg(wname);
        if (dlg.DoModal(nullptr) == IDOK)
        {
            std::string newName = Str(dlg.GetName());
            //m_logFile[m_logLines[item].line].processName = newName;
            // todo: loop through m_logFile and change all processes with ProcessID -> new name
            // but also new lines from that PID should get this name....
        }
    }
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
    {
        return;
    }

    m_filter.messageFilters.emplace_back(Str(m_highlightText), MatchType::Simple, filterType, bgColor, fgColor);
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
    {
        return;
    }

    ToggleBookmark(item);
}

void CLogView::FindBookmark(int direction)
{
    int line = FindLine([](const LogLine& line) { return line.bookmark; }, direction);
    if (line >= 0)
    {
        ScrollToIndex(line, false);
    }
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
    {
        line.bookmark = false;
    }
    Invalidate();
}

#ifndef _WIN64
LRESULT CLogView::OnCustomDraw(NMHDR*)
{
    return CDRF_DODEFAULT;
}
#else
LRESULT CLogView::OnCustomDraw(NMHDR* pnmh)
{
    auto lplvcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(pnmh);
    if (lplvcd->nmcd.dwDrawStage == CDDS_PREPAINT)
    {
        return CDRF_NOTIFYPOSTPAINT;
    }

    if (lplvcd->nmcd.dwDrawStage == CDDS_POSTPAINT)
    {
        m_hdr.Windows10Workaround();
    }
    return CDRF_SKIPDEFAULT;
}
#endif

LRESULT CLogView::DoPaint(CDCHandle dc)
{
    RECT rect;
    dc.GetClipBox(&rect);
    dc.FillSolidRect(&rect, Colors::BackGround);
    DefWindowProc(WM_PAINT, reinterpret_cast<WPARAM>(dc.m_hDC), 0);
    return CDRF_SKIPPOSTPAINT;
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
    {
        ScrollDown();
    }
}

bool CLogView::GetAutoScrollStop() const
{
    return m_autoScrollStop;
}

void CLogView::SetAutoScrollStop(bool enable)
{
    m_autoScrollStop = enable;
}

const std::vector<ColumnInfo>& CLogView::GetColumns() const
{
    return m_columns;
}

void CLogView::Clear()
{
    m_firstLine = m_logFile.Count();
    SetItemCount(0);
    m_dirty = false;
    m_logLines.clear();
    m_highlightText.clear();
    if (m_autoScrollStop)
    {
        m_autoScrollDown = true;
    }

    ResetFilters();
}

int CLogView::GetFocusLine() const
{
    int item = GetNextItem(-1, LVNI_FOCUSED);
    if (item < 0)
    {
        return -1;
    }

    return m_logLines[item].line;
}

void CLogView::SetFocusLine(int line)
{
    auto it = std::upper_bound(m_logLines.begin(), m_logLines.end(), line, [](int line, const LogLine& logLine) { return line < logLine.line; });
    ScrollToIndex(static_cast<int>(it - m_logLines.begin() - 1), false);
}

void CLogView::Add(int beginIndex, int line, const Message& msg)
{
    if (IsClearMessage(msg))
    {
        Clear();
    }

    if (!IsIncluded(msg))
    {
        return;
    }

    if (IsBeepMessage(msg))
    {
        MessageBeep(0xFFFFFFFF); // A simple beep. If the sound card is not available, the sound is generated using the speaker.
    }

    m_dirty = true;
    m_changed = true;
    auto it = m_logLines.begin();
    while (it != m_logLines.end() && it->line < beginIndex)
    {
        ++it;
    }
    m_logLines.erase(m_logLines.begin(), it);

    int viewline = static_cast<int>(m_logLines.size());

    LogLine logline(line);
    logline.bookmark = MatchFilterType(FilterType::Bookmark, msg);
    m_logLines.push_back(logline);

    if (m_autoScrollDown && MatchFilterType(FilterType::Stop, msg))
    {
        m_stop = [this, viewline]() {
            StopScrolling();
            ScrollToIndex(viewline, true);
        };
        return;
    }

    if (MatchFilterType(FilterType::Track, msg))
    {
        m_autoScrollDown = false;
        m_track = [this, viewline]() {
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
        SetItemCountEx(static_cast<int>(m_logLines.size()), LVSICF_NOSCROLL);
        if (m_autoScrollDown)
        {
            ScrollDown();
        }

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
        {
            break;
        }
        SetItemState(item, 0, LVIS_SELECTED);
    }
}

// returns false if centering was requested but not executed
//         because there where not enough lines below the requested index
//           and it can be usefull to call ScrollToIndex again when more lines are available
bool CLogView::ScrollToIndex(int index, bool center)
{
    if (index < 0 || index >= static_cast<int>(m_logLines.size()))
    {
        return true;
    }

    ClearSelection();
    SetItemState(index, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

    int paddingLines = GetCountPerPage() / 2;
    int minTopIndex = std::max(0, index - paddingLines);

    // this ensures the line is visible and not at the top of the view
    // if it does not need to be, also when coming from a higher index
    EnsureVisible(minTopIndex, 0);
    EnsureVisible(index, 0);

    if (index > paddingLines)
    {
        // if there are more items above the index then half a page, then centering may be possible.
        if (center)
        {
            int maxBottomIndex = std::min<int>(static_cast<int>(m_logLines.size() - 1), index + paddingLines);
            EnsureVisible(maxBottomIndex, 0);
            return (maxBottomIndex == (index + paddingLines));
        }
    }
    return true;
}

void CLogView::ScrollDown()
{
    ScrollToIndex(static_cast<int>(m_logLines.size() - 1), false);
}

bool CLogView::GetClockTime() const
{
    return m_clockTime;
}

void CLogView::SetClockTime(bool clockTime)
{
    m_clockTime = clockTime;
    Invalidate(0);
}

void CLogView::SetViewProcessColors(bool value)
{
    m_processColors = value;
    Invalidate(0);
}

bool CLogView::GetViewProcessColors() const
{
    return m_processColors;
}

void CLogView::SelectAll()
{
    int lines = GetItemCount();
    for (int i = 0; i < lines; ++i)
    {
        SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
    }
}

std::wstring CLogView::GetItemWText(int item, int subItem) const
{
    CComBSTR bstr;
    GetItemText(item, subItem, bstr.m_str);
    return std::wstring(bstr.m_str, bstr.m_str + bstr.Length());
}

std::wstring CLogView::GetLineAsText(int item) const
{
    return GetColumnText(item, Column::Line) + L"\t" + GetColumnText(item, Column::Time) + L"\t" + GetColumnText(item, Column::Pid) + L"\t" + GetColumnText(item, Column::Process) + L"\t" + GetColumnText(item, Column::Message);
}

Win32::HGlobal MakeGlobalString(std::string_view str)
{
    Win32::HGlobal handle(GlobalAlloc(GMEM_MOVEABLE, str.size() + 1));
    Win32::GlobalLock<char> lock(handle);

    memcpy(lock.Ptr(), str.data(), str.size());
    lock.Ptr()[str.size()] = '\0';
    return handle;
}

Win32::HGlobal MakeGlobalWideString(std::wstring_view str)
{
    auto charbytes = str.size() * sizeof(str[0]);
    auto allocsize = (str.size() + 1) * sizeof(str[0]);
    Win32::HGlobal handle(GlobalAlloc(GMEM_MOVEABLE, allocsize));
    Win32::GlobalLock<wchar_t> lock(handle);
    memcpy(lock.Ptr(), str.data(), charbytes);
    lock.Ptr()[str.size()] = '\0';
    return handle;
}

void CLogView::CopyToClipboard(std::wstring_view str)
{
    if (OpenClipboard() != 0)
    {
        EmptyClipboard();
        auto gstr = MakeGlobalWideString(str);
        SetClipboardData(CF_UNICODETEXT, gstr.release());
        CloseClipboard();
    }
}

void CLogView::CopyMessagesToClipboard()
{
    CopyToClipboard(GetSelectedMessagesAsWString());
}

std::wstring CLogView::GetSelectedLines() const
{
    std::wostringstream ss;
    int item = -1;
    while ((item = GetNextItem(item, LVNI_ALL | LVNI_SELECTED)) >= 0)
    {
        ss << GetLineAsText(item) << "\r\n";
    }
    return ss.str();
}

std::wstring CLogView::GetSelectedMessagesAsWString() const
{
    std::wostringstream ss;
    int item = -1;
    while ((item = GetNextItem(item, LVNI_ALL | LVNI_SELECTED)) >= 0)
    {
        ss << GetColumnText(item, Column::Message) << "\r\n";
    }
    return ss.str();
}

void CLogView::Copy()
{
    if (m_highlightText.empty())
    {
        CopyToClipboard(GetSelectedLines());
    }
    else
    {
        CopyToClipboard(m_highlightText);
    }
}

std::wstring CLogView::GetHighlightText() const
{
    return m_highlightText;
}

void CLogView::SetHighlightText(std::wstring_view text)
{
    if (m_highlightText != text)
    {
        m_highlightText = text;
        Invalidate(0);
        SetFocus();
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
    {
        return -1;
    }

    auto size = static_cast<int>(m_logLines.size());
    do
    {
        line += direction;
        if (line < 0)
        {
            line += size;
        }

        if (line >= size)
        {
            line -= size;
        }

        if (pred(m_logLines[line]))
        {
            return static_cast<int>(line);
        }
    } while (line != begin);

    return -1;
}

bool CLogView::Find(std::wstring_view text, int direction)
{
    StopTracking();

    int line = FindLine([text, this](const LogLine& line) { return Contains(m_logFile[line.line].text, Str(text)); }, direction);
    if (line < 0)
    {
        return false;
    }

    bool sameLine = GetItemState(line, LVIS_FOCUSED) != 0;
    if (!sameLine)
    {
        ScrollToIndex(line, true);
    }

    if (sameLine && text == m_highlightText)
    {
        return false;
    }

    SetHighlightText(text);
    return true;
}

bool CLogView::FindNext(std::wstring_view text)
{
    return Find(text, +1);
}

bool CLogView::FindPrevious(std::wstring_view text)
{
    return Find(text, -1);
}

boost::property_tree::ptree MakePTree(const std::vector<ColumnInfo>& columns)
{
    boost::property_tree::ptree pt;
    for (int i = 0; i < Column::Count; ++i)
    {
        const ColumnInfo& col = columns[i];
        boost::property_tree::ptree colPt;
        colPt.put("Index", i);
        colPt.put("Enable", col.enable);
        colPt.put("Width", col.column.cx);
        colPt.put("Order", col.column.iOrder);
        pt.add_child("Column", colPt);
    }
    return pt;
}

void CLogView::ReadColumns(const boost::property_tree::ptree& pt)
{
    for (auto& item : pt)
    {
        if (item.first == "Column")
        {
            const auto& colPt = item.second;
            auto index = colPt.get_optional<size_t>("Index");
            if (index && *index < m_columns.size())
            {
                ColumnInfo& col = m_columns[*index];

                auto enable = colPt.get_optional<bool>("Enable");
                if (enable)
                {
                    col.enable = *enable;
                }

                auto width = colPt.get_optional<int>("Width");
                if (width)
                {
                    col.column.cx = *width;
                }

                auto order = colPt.get_optional<int>("Order");
                if (order)
                {
                    col.column.iOrder = *order;
                }
            }
        }
    }
    UpdateColumns();
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
        {
            break;
        }

        ColumnInfo column = m_columns[i];
        column.enable = Win32::RegGetDWORDValue(regColumn, L"Enable", static_cast<DWORD>(column.enable)) != 0;
        column.column.cx = Win32::RegGetDWORDValue(regColumn, L"Width", column.column.cx);
        column.column.iOrder = Win32::RegGetDWORDValue(regColumn, L"Order", column.column.iOrder);
        columns.push_back(column);
    }
    if (columns.size() == m_columns.size())
    {
        m_columns.swap(columns);
    }

    CRegKey regFilters;
    if (regFilters.Open(reg, L"MessageFilters") == ERROR_SUCCESS)
    {
        LoadFilterSettings(m_filter.messageFilters, regFilters);
    }
    if (regFilters.Open(reg, L"ProcessFilters") == ERROR_SUCCESS)
    {
        LoadFilterSettings(m_filter.processFilters, regFilters);
    }

    ApplyFilters();
    UpdateColumns();
}

void CLogView::SaveSettings(CRegKey& reg)
{
    reg.SetDWORDValue(L"AutoScrollStop", static_cast<DWORD>(GetAutoScrollStop()));
    reg.SetDWORDValue(L"ClockTime", static_cast<DWORD>(GetClockTime()));
    reg.SetDWORDValue(L"ShowProcessColors", static_cast<DWORD>(GetViewProcessColors()));

    int i = 0;
    for (auto& col : m_columns)
    {
        CRegKey regColumn;
        regColumn.Create(reg, WStr(wstringbuilder() << L"Columns\\Column" << i));
        regColumn.SetDWORDValue(L"Enable", static_cast<DWORD>(col.enable));
        regColumn.SetDWORDValue(L"Width", col.column.cx);
        regColumn.SetDWORDValue(L"Order", col.column.iOrder);
        ++i;
    }

    CRegKey regFilters;
    if (regFilters.Create(reg, L"MessageFilters") == ERROR_SUCCESS)
    {
        SaveFilterSettings(m_filter.messageFilters, regFilters);
    }
    if (regFilters.Create(reg, L"ProcessFilters") == ERROR_SUCCESS)
    {
        SaveFilterSettings(m_filter.processFilters, regFilters);
    }
}

void CLogView::SaveSelection(const std::wstring& fileName) const
{
    if (!m_highlightText.empty())
    {
        return; // token selection is invalid input for SaveSelection
    }

    std::ofstream fs;
    OpenLogFile(fs, fileName);


    int item = -1;
    while ((item = GetNextItem(item, LVNI_ALL | LVNI_SELECTED)) >= 0)
    {
        int line = m_logLines[item].line;
        const Message& msg = m_logFile[line];
        WriteLogFileMessage(fs, msg.time, msg.systemTime, msg.processId, msg.processName, msg.text);
    }

    fs.close();
    if (!fs)
    {
        Win32::ThrowLastError(fileName);
    }
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
    {
        Win32::ThrowLastError(filename);
    }
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
    {
        if (line.bookmark)
        {
            bookmarks.push_back(line.line);
        }
    }
    return bookmarks;
}

void CLogView::ResetFilters()
{
    for (auto& filter : m_filter.messageFilters)
    {
        if (filter.filterType == FilterType::Once)
        {
            filter.matched = false;
        }
    }
    for (auto& filter : m_filter.processFilters)
    {
        if (filter.filterType == FilterType::Once)
        {
            filter.matched = false;
        }
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
    //    logLines.reserve(m_logLines.size());
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
            {
                focusItem = item;
            }

            ++item;
        }
        ++line;
    }

    m_logLines.swap(logLines);
    SetItemCountEx(static_cast<int>(m_logLines.size()), LVSICF_NOSCROLL);
    ScrollToIndex(focusItem, false);
    SetItemState(focusItem, LVIS_FOCUSED, LVIS_FOCUSED);
    EndUpdate();
}

bool FilterSupportsColor(FilterType::type value)
{
    switch (value)
    {
    case FilterType::Highlight:
    case FilterType::Track:
    case FilterType::Stop:
    case FilterType::Once:
        return true;
    default: break;
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
                {
                    return TextColor(it->second, Colors::Text);
                }
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
        {
            return TextColor(filter.bgColor, filter.fgColor);
        }
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
    return MatchFilterType(m_filter.messageFilters, type, msg.text) ||
           MatchFilterType(m_filter.processFilters, type, msg.processName);
}

} // namespace debugviewpp
} // namespace fusion
