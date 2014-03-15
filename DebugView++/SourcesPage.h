// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <regex>
#include <vector>

#include "Grid.h"
#include "DebugView++Lib/FilterType.h"
#include "DebugView++Lib/SourceType.h"
#include "DebugView++Lib/Filter.h"
#include "Resource.h"
#include "DebugView++Lib/LogSources.h"


namespace fusion {
namespace debugviewpp {

class CSourcesPageImpl :
	public CDialogImpl<CSourcesPageImpl>,
	public CDialogResize<CSourcesPageImpl>
{
public:
	CSourcesPageImpl();

	std::vector<std::shared_ptr<LogSource>> GetSourcesToDelete() const;
	void SetLogSources(const std::vector<std::shared_ptr<LogSource>>& logsources);

	enum { IDD = IDD_SOURCES_PAGE };

	BEGIN_DLGRESIZE_MAP(CSourcesPageImpl)
		DLGRESIZE_CONTROL(IDC_SOURCES_GRID, DLSZ_SIZE_X | DLSZ_SIZE_Y)
	END_DLGRESIZE_MAP()

	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID);
	void ExceptionHandler();

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnDestroy();

private:
	void AddFilter(const Filter& filter);
	bool GetSourceEnable(int iItem) const;
	std::wstring GetSourceText(int iItem) const;
	SourceType::type GetSourceType(int iItem) const;
	void UpdateGrid();

	LRESULT OnAddItem(NMHDR* pnmh);
	LRESULT OnClickItem(NMHDR* pnmh);
	LRESULT OnItemChanged(NMHDR* pnmh);

	const FilterType::type* m_filterTypes;
	size_t m_filterTypeCount;
	const MatchType::type* m_matchTypes;
	size_t m_matchTypeCount;
	CPropertyGridCtrl m_grid;
	
	std::vector<std::shared_ptr<LogSource>> m_logsources;
	std::vector<std::shared_ptr<LogSource>> m_deleteSources;
};

class CSourcesPage : public CSourcesPageImpl
{
public:
	template <size_t FilterTypeCount, size_t MatchTypeCount>
	CSourcesPage() :
		CSourcesPageImpl()
	{
	}
};

} // namespace debugviewpp 
} // namespace fusion
