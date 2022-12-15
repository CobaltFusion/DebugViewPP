// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#pragma once

#include <vector>
#include <deque>
#include <boost/property_tree/ptree_fwd.hpp>
#include "Win32/Window.h"
#include "Win32/Win32Lib.h"
#include "CobaltFusion/AtlWinExt.h"
#include "CobaltFusion/stringbuilder.h"
#include "DebugView++Lib/LogFile.h"
#include "FilterDlg.h"
#include "DropTargetSupport.h"
#include "Win32/Com.h"

namespace fusion {
namespace debugviewpp {

class CMainFrame;

struct SelectionInfo
{
    SelectionInfo();
    SelectionInfo(int beginLine, int endLine, int count);

    int beginLine = 0;
    int endLine = 0;
    int count = 0;
};

struct TextColor
{
    TextColor(COLORREF back, COLORREF fore);

    COLORREF back;
    COLORREF fore;
};

struct Highlight
{
    Highlight(int id, int begin, int end, const TextColor& color);

    int id;
    int begin;
    int end;
    TextColor color;
};

struct LogLine
{
    explicit LogLine(int line);

    bool bookmark;
    int line;
};

struct Column
{
    enum type
    {
        Bookmark = 0,
        Line,
        Date,
        Time,
        Pid,
        Process,
        Message,
        Count
    };
};

struct ItemData
{
    ItemData();

    std::wstring text[Column::Count];
    TextColor color;
    std::vector<Highlight> highlights;
};

struct ColumnInfo
{
    bool enable;
    LVCOLUMN column;
};

boost::property_tree::ptree MakePTree(const std::vector<ColumnInfo>& columns);

class CMyHeaderCtrl : public CWindowImpl<CMyHeaderCtrl, CHeaderCtrl>
{
public:
    BEGIN_MSG_MAP_EX(CMyHeaderCtrl)
        REFLECTED_NOTIFY_CODE_HANDLER_EX(HDN_BEGINTRACK, BlockColumn0)
        REFLECTED_NOTIFY_CODE_HANDLER_EX(HDN_ENDTRACK, OnEndDrag)
    END_MSG_MAP()

    // on windows 10 listview headers are missing depth, we manually draw them to compensate
    void Windows10Workaround()
    {
        CDC dc(GetDC());
        RECT rect;
        dc.GetClipBox(&rect);

        CPen graypen(CreatePen(PS_SOLID, 1, RGB(208, 212, 208)));
        dc.SelectPen(graypen);
        dc.MoveTo(rect.left, rect.bottom - 2);
        dc.LineTo(rect.right, rect.bottom - 2);
    }

    void TriggerHeaderRedraw()
    {
        GetParent().Invalidate();
    }

    LRESULT OnEndDrag(NMHDR* pnmh)
    {
        TriggerHeaderRedraw();
        return BlockColumn0(pnmh);
    }

    LRESULT BlockColumn0(NMHDR* pnmh)
    {
        auto& nmhdr = *reinterpret_cast<NMHEADER*>(pnmh);
        if (nmhdr.iItem == 0)
        {
            return TRUE;
        }

        HDHITTESTINFO info;
        info.pt = Win32::GetMessagePos();
        ScreenToClient(&info.pt);
        return static_cast<LRESULT>(HitTest(&info) < 1);
    }
};


class CLogView : public CDoubleBufferWindowImpl<CLogView, CListViewCtrl,
                     CWinTraitsOR<
                         LVS_OWNERDRAWFIXED | LVS_REPORT | LVS_OWNERDATA | LVS_NOSORTHEADER | LVS_SHOWSELALWAYS,
                         LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_HEADERDRAGDROP>>,
                 public COwnerDraw<CLogView>,
                 public ExceptionHandler<CLogView, std::exception>
{
public:
    CLogView(std::wstring name, CMainFrame& mainFrame, LogFile& logFile, LogFilter logFilter = LogFilter());
    ~CLogView() override;
    DECLARE_WND_SUPERCLASS(nullptr, CListViewCtrl::GetWndClassName())

    LRESULT DoPaint(CDCHandle dc);

    std::wstring GetName() const;
    void SetName(const std::wstring& name);
    void SetFont(HFONT hFont);
    bool GetAutoScroll() const;
    void SetAutoScroll(bool enable);
    bool GetAutoScrollStop() const;
    void SetAutoScrollStop(bool enable);
    const std::vector<ColumnInfo>& GetColumns() const;
    void ReadColumns(const boost::property_tree::ptree& pt);
    void Clear();
    int GetFocusLine() const;
    void SetFocusLine(int line);
    void Add(int beginIndex, int line, const Message& msg);
    void BeginUpdate();
    bool EndUpdate();
    void ClearSelection();
    void StopTracking();
    void StopScrolling();
    bool ScrollToIndex(int index, bool center);
    void ScrollDown();

    bool IsColumnViewed(int nID) const;
    bool GetClockTime() const;
    void SetClockTime(bool clockTime);
    void SetViewProcessColors(bool value);
    bool GetViewProcessColors() const;
    bool GetBookmark() const;
    void SelectAll();
    void Copy();

    std::wstring GetHighlightText() const;
    void SetHighlightText(std::wstring_view text);
    bool FindNext(std::wstring_view text);
    bool FindPrevious(std::wstring_view text);

    LogFilter GetFilters() const;
    void SetFilters(const LogFilter& filter);

    using CListViewCtrl::GetItemText;
    std::wstring GetLineAsText(int item) const;
    std::wstring GetItemWText(int item, int subItem) const;

    void LoadSettings(CRegKey& reg);
    void SaveSettings(CRegKey& reg);
    void Save(const std::wstring& fileName) const;
    void SaveSelection(const std::wstring& fileName) const;

    SelectionInfo GetViewRange() const;
    SelectionInfo GetSelectedRange() const;

    void MeasureItem(MEASUREITEMSTRUCT* pMeasureItemStruct) const;
    void DrawItem(DRAWITEMSTRUCT* pDrawItemStruct) const;
    void DeleteItem(DELETEITEMSTRUCT* lParam);
    void ResetToLine(int line);
    void CopyToClipboard(std::wstring_view str);
    void CopyMessagesToClipboard();
    std::wstring GetSelectedMessagesAsWString() const;

private:
    DECLARE_MSG_MAP()

    void OnException() const;
    void OnException(const std::exception& ex);
    LRESULT OnCreate(const CREATESTRUCT* pCreate);
    void OnContextMenu(HWND hWnd, CPoint pt);
    void OnLButtonDown(UINT flags, CPoint point);
    void OnMouseMove(UINT flags, CPoint point);
    void OnLButtonUp(UINT nFlags, CPoint point);
    void OnTimer(UINT_PTR nIDEvent);
    BOOL OnSetCursor(CWindow wnd, UINT nHitTest, UINT message);
    LRESULT OnGetDispInfo(NMHDR* pnmh);
    LRESULT OnClick(NMHDR* pnmh);
    LRESULT OnDblClick(NMHDR* pnmh);
    LRESULT OnItemChanged(NMHDR* pnmh);
    LRESULT OnIncrementalSearch(NMHDR* pnmh);
    LRESULT OnOdCacheHint(NMHDR* pnmh);
    LRESULT OnBeginDrag(NMHDR* pnmh);
    void OnViewClear(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewReset(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewResetToLine(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewExcludeLines(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewSelectAll(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewCopyMessages(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewCopy(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewAutoScroll(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewAutoScrollStop(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewTime(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewProcessColors(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnEscapeKey(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewFindNext(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewFindPrevious(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewNextProcess(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewPreviousProcess(UINT uNotifyCode, int nID, CWindow wndCtl);
    void AddProcessFilter(FilterType::type filterType, COLORREF bgColor = RGB(255, 255, 255), COLORREF fgColor = RGB(0, 0, 0));
    void OnViewProcessRename(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewProcessHighlight(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewProcessInclude(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewProcessExclude(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewProcessTrack(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewProcessOnce(UINT uNotifyCode, int nID, CWindow wndCtl);
    void AddMessageFilter(FilterType::type filterType, COLORREF bgColor = RGB(255, 255, 255), COLORREF fgColor = RGB(0, 0, 0));
    void OnViewFilterHighlight(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewFilterInclude(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewFilterExclude(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewFilterToken(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewFilterTrack(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewFilterOnce(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewBookmark(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewNextBookmark(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewPreviousBookmark(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewClearBookmarks(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnViewColumn(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    LRESULT OnCustomDraw(NMHDR* pnmh);

    std::vector<std::string> GetSelectedMessages() const;
    std::wstring GetSelectedLines() const;
    void UpdateColumns();
    int ColumnToSubItem(Column::type column) const;
    Column::type SubItemToColumn(int iSubItem) const;
    int GetTextIndex(int iItem, int xPos) const;
    int GetTextIndex(CDCHandle dc, int iItem, int xPos) const;
    int TextHighlightHitTest(int iItem, const POINT& pt);
    std::wstring GetColumnText(int iItem, Column::type column) const;
    RECT GetItemRect(int iItem, unsigned code) const;
    RECT GetSubItemRect(int iItem, int iSubItem, unsigned code) const;
    void DrawItem(CDCHandle dc, int iItem, unsigned iItemState) const;
    Highlight GetSelectionHighlight(CDCHandle dc, int iItem) const;
    std::vector<Highlight> GetHighlights(std::wstring_view text) const;
    void DrawBookmark(CDCHandle dc, int iItem) const;
    void DrawSubItem(CDCHandle dc, int iItem, int iSubItem, const ItemData& data) const;

    ItemData GetItemData(int iItem) const;

    std::vector<int> GetBookmarks() const;
    void ToggleBookmark(int iItem);
    void FindBookmark(int direction);

    template <typename Predicate>
    int FindLine(Predicate pred, int direction) const;

    bool Find(std::wstring_view text, int direction);
    bool FindProcess(int direction);
    void ApplyFilters();
    bool IsClearMessage(const Message& msg) const;
    bool IsBeepMessage(const Message& msg) const;
    bool IsIncluded(const Message& msg);
    bool MatchFilterType(FilterType::type type, const Message& msg) const;
    TextColor GetTextColor(const Message& msg) const;
    void ResetFilters();

    std::wstring m_name;
    CMainFrame& m_mainFrame;
    LogFile& m_logFile;
    LogFilter m_filter;
    MatchColors m_matchColors;
    CMyHeaderCtrl m_hdr;
    std::vector<ColumnInfo> m_columns;
    int m_firstLine;
    std::deque<LogLine> m_logLines;
    bool m_clockTime;
    bool m_processColors;
    bool m_autoScrollDown;
    bool m_autoScrollStop;
    bool m_dirty;
    bool m_changed;
    std::function<void()> m_stop;
    std::function<bool()> m_track;
    Win32::HIcon m_hBookmarkIcon;
    std::wstring m_highlightText;
    HCURSOR m_hBeamCursor;
    CPoint m_dragStart;
    CPoint m_dragEnd;
    bool m_dragging;
    int m_scrollX;
    std::wstring m_dispInfoText;
    Win32::ComObjectPtr<DropTargetSupport> m_pDropTargetSupport;
};

} // namespace debugviewpp
} // namespace fusion
