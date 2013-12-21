// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#pragma warning(push, 3)
#include "PropertyGrid.h"
#pragma warning(pop)

#include "Resource.h"
#include "Utilities.h"

namespace fusion {

struct ProcessFilter
{
};

class CProcessFilterPage :
	public CDialogImpl<CProcessFilterPage>,
	public CDialogResize<CProcessFilterPage>
{
public:
	enum { IDD = IDD_FILTER_PAGE };

	BEGIN_DLGRESIZE_MAP(CProcessFilterPage)
		DLGRESIZE_CONTROL(IDC_GRID, DLSZ_SIZE_X | DLSZ_SIZE_Y)
	END_DLGRESIZE_MAP()

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnDestroy();

	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID);
	void ExceptionHandler();

private:
	CPropertyGridCtrl m_grid;
};

} // namespace fusion
