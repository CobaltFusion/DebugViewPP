// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <vector>

#include "Grid.h"
#include "PropertyColorItem.h"
#include "Resource.h"
#include "Filter.h"
#include "DebugView++Lib/LogSources.h"

namespace fusion {
namespace debugviewpp {

class CSourcesDlg :
	public CDialogImpl<CSourcesDlg>,
	public CDialogResize<CSourcesDlg>
{
public:
	CSourcesDlg(std::vector<std::shared_ptr<LogSource>> logsources);
	
	enum { IDD = IDD_SOURCES };

	BEGIN_DLGRESIZE_MAP(CSourcesDlg)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_SOURCES_GRID, DLSZ_SIZE_X | DLSZ_SIZE_Y)
	END_DLGRESIZE_MAP()

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnOk(UINT uNotifyCode, int nID, CWindow wndCtl);
	LRESULT OnClickItem(NMHDR* pnmh);
	
	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID);
	void ExceptionHandler();
	std::vector<std::shared_ptr<LogSource>> GetSourcesToRemove();

private:
	bool GetSourceEnable(int iItem) const;
	std::wstring GetSourceText(int iItem) const;
	SourceType::type GetSourceType(int iItem) const;

	CPropertyGridCtrl m_grid;
	std::vector<std::shared_ptr<LogSource>> m_logsources;
	std::vector<std::shared_ptr<LogSource>> m_logsourcesToRemove;
};

} // namespace debugviewpp 
} // namespace fusion
