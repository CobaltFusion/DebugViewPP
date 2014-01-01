// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <vector>
#include "OffscreenPaint.h"
#include "Win32Lib.h"
#include "LogFile.h"
#include "FilterDlg.h"
#include "DisplayInfo.h"
#include "ProcessInfo.h"

namespace fusion {
namespace debugviewpp {

typedef CWinTraitsOR<LVS_REPORT | LVS_OWNERDATA | LVS_NOSORTHEADER | LVS_SHOWSELALWAYS> CListViewTraits;

class CMainFrame;

struct SelectionInfo
{
	SelectionInfo();
	SelectionInfo(int beginLine, int endLine, int count);

	int beginLine;
	int endLine;
	int count;
};

struct TextColor
{
	TextColor(COLORREF back, COLORREF fore);

	COLORREF back;
	COLORREF fore;
};

struct Highlight
{
	Highlight(int begin, int end, const TextColor& color);

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

struct ItemData
{
	ItemData();

	std::wstring text[6];
	TextColor color;
	std::vector<Highlight> highlights;
};

struct ColumnInfo
{
	bool enable;
	LVCOLUMN column;
};

class CMyHeaderCtrl : public CWindowImpl<CMyHeaderCtrl, CHeaderCtrl>
{
public:
	BEGIN_MSG_MAP_EX(CMyHeaderCtrl)
		REFLECTED_NOTIFY_CODE_HANDLER_EX(HDN_BEGINDRAG, BlockColumn0)
		REFLECTED_NOTIFY_CODE_HANDLER_EX(HDN_ENDDRAG, BlockColumn0)
		REFLECTED_NOTIFY_CODE_HANDLER_EX(HDN_BEGINTRACK, BlockColumn0)
	END_MSG_MAP()

	LRESULT BlockColumn0(NMHDR* pnmh)
	{
		auto& nmhdr = *reinterpret_cast<NMHEADER*>(pnmh);
		if (nmhdr.iItem == 0)
			return TRUE;

		HDHITTESTINFO info;
		auto pos = GetMessagePos();
		info.pt = CPoint(GET_X_LPARAM(pos), GET_Y_LPARAM(pos));
		ScreenToClient(&info.pt);
		return HitTest(&info) < 1;
	}
};

struct Column
{
	enum type
	{
		Bookmark = 0,
		Line,
		Time,
		Pid,
		Process,
		Message,
		Count
	};
};

class CLogView :
	public CWindowImpl<CLogView, CListViewCtrl, CListViewTraits>,
	public COffscreenPaint<CLogView>
{
public:
	CLogView(CMainFrame& mainFrame, LogFile& logFile, LogFilter logFilter = LogFilter());

	DECLARE_WND_SUPERCLASS(nullptr, CListViewCtrl::GetWndClassName())

	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID);
	void ExceptionHandler();

	void DoPaint(CDCHandle dc, const RECT& rcClip);

	bool GetScroll() const;
	void SetScroll(bool enable);
	void Clear();
	int GetFocusLine() const;
	void SetFocusLine(int line);
	void Add(int line, const Message& msg);
	void BeginUpdate();
	int EndUpdate();
	void ClearSelection();

	void StopScrolling();
	void ScrollToIndex(int index, bool center);
	void ScrollDown();
	bool IsLastLineSelected();

	bool IsColumnViewed(int nID) const;
	bool GetClockTime() const;
	void SetClockTime(bool clockTime);
	bool GetBookmark() const;
	void SelectAll();
	void Copy();

	std::wstring GetHighlightText() const;
	void SetHighlightText(const std::wstring& text = std::wstring());
	bool FindNext(const std::wstring& text);
	bool FindPrevious(const std::wstring& text);

	LogFilter GetFilters() const;
	void SetFilters(const LogFilter& filter);

	using CListViewCtrl::GetItemText;
	std::string GetItemText(int item, int subItem) const;
	std::string GetItemText(int item) const;
	std::wstring GetItemWText(int item, int subItem) const;

	void LoadSettings(CRegKey& reg);
	void SaveSettings(CRegKey& reg);
	void Save(const std::wstring& fileName) const;
	
	SelectionInfo GetViewRange() const;
	SelectionInfo GetSelectedRange() const;

private:
	LRESULT OnCreate(const CREATESTRUCT* pCreate);
	void OnContextMenu(HWND hWnd, CPoint pt);
	void OnLButtonDown(UINT nFlags, CPoint point);
	void OnMouseMove(UINT flags, CPoint point);
	void OnLButtonUp(UINT nFlags, CPoint point);
	LRESULT OnGetDispInfo(NMHDR* pnmh);
	LRESULT OnClick(NMHDR* pnmh);
	LRESULT OnDblClick(NMHDR* pnmh);
	LRESULT OnItemChanged(NMHDR* pnmh);
	LRESULT OnCustomDraw(NMHDR* pnmh);
	LRESULT OnOdStateChanged(NMHDR* pnmh);
	LRESULT OnIncrementalSearch(NMHDR* pnmh);
	LRESULT OnOdCacheHint(NMHDR* pnmh);
	void OnViewClear(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnViewSelectAll(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnViewCopy(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnViewScroll(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnViewTime(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnViewHideHighlight(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnViewFindNext(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnViewFindPrevious(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnViewNextProcess(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnViewPreviousProcess(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnViewExclude(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnViewBookmark(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnViewNextBookmark(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnViewPreviousBookmark(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnViewClearBookmarks(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnViewColumn(UINT uNotifyCode, int nID, CWindow wndCtl);

	void UpdateColumnWidths();
	void UpdateColumns();
	int ColumnToSubItem(Column::type column) const;
	Column::type SubItemToColumn(int iSubItem) const;
	int GetTextIndex(int iItem, int xPos);
	int GetTextIndex(CDCHandle dc, int iItem, int xPos) const;
	std::string GetColumnText(int iItem, Column::type column) const;
	RECT GetItemRect(int iItem, unsigned code) const;
	RECT GetSubItemRect(int iItem, int iSubItem, unsigned code) const;
	void DrawItem(CDCHandle dc, int iItem, unsigned iItemState) const;
	Highlight GetSelectionHighlight(CDCHandle dc, int iItem) const;
	std::vector<Highlight> GetHighlights(const std::string& text) const;
	void DrawBookmark(CDCHandle dc, int iItem) const;
	void DrawSubItem(CDCHandle dc, int iItem, int iSubItem, const ItemData& data) const;

	ItemData GetItemData(int iItem) const;

	std::vector<int> GetBookmarks() const;
	void ToggleBookmark(int iItem);
	void FindBookmark(int direction);
	bool Find(const std::string& text, int direction);
	void ApplyFilters();
	bool IsProcessIncluded(const std::string& msg) const;
	bool IsMessageIncluded(const std::string& msg) const;
	bool IsIncluded(const Message& msg) const;
	bool IsStop(const std::string& text) const;
	bool IsTrack(const std::string& text) const;
	TextColor GetTextColor(const std::string& text) const;

	CMainFrame& m_mainFrame;
	LogFile& m_logFile;
	LogFilter m_filter;
	CMyHeaderCtrl m_hdr;
	std::vector<ColumnInfo> m_columns;
	int m_firstLine;
	std::vector<LogLine> m_logLines;
	bool m_clockTime;
	bool m_autoScrollDown;
	bool m_dirty;
	int m_addedLines;
	std::function<void ()> m_stop;
	std::function<void ()> m_track;
	bool m_insidePaint;
	HIcon m_hBookmarkIcon;
	std::wstring m_highlightText;
	CPoint m_dragStart;
	CPoint m_dragEnd;
};

} // namespace debugviewpp 
} // namespace fusion
