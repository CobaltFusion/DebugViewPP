// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "FilterPage.h"
#include "PropertyColorItem.h"
#include "RegExDlg.h"
#include "resource.h"
#include "Filter.h"

#include "CobaltFusion/AtlWinExt.h"
#include "atleverything.h"

#include <vector>

namespace fusion {
namespace debugviewpp {

class CFilterDlg : public CDialogImpl<CFilterDlg>,
                   public CDialogResize<CFilterDlg>,
                   public ExceptionHandler<CFilterDlg, std::exception>
{
public:
    explicit CFilterDlg(std::wstring name, const LogFilter& filter = LogFilter());

    std::wstring GetName() const;
    LogFilter GetFilters() const;

    enum
    {
        IDD = IDD_FILTER
    };

    BEGIN_DLGRESIZE_MAP(CFilterDlg)
        DLGRESIZE_CONTROL(IDC_TAB, DLSZ_SIZE_X | DLSZ_SIZE_Y)
        DLGRESIZE_CONTROL(IDC_FILTER_SAVE, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_FILTER_LOAD, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_FILTER_REGEX, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_FILTER_REMOVEALL, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
    END_DLGRESIZE_MAP()

    DECLARE_MSG_MAP()

    void OnException();
    void OnException(const std::exception& ex);
    BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
    void OnDestroy();
    void OnSize(UINT nType, CSize size);

    void OnSave(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/);
    void OnLoad(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/);
    void OnRegEx(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/);
    void OnClearAll(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/);
    void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnOk(UINT uNotifyCode, int nID, CWindow wndCtl);
    LRESULT OnTabSelChange(NMHDR* pnmh);
    void SelectTab(int tab);

private:
    CTabCtrl m_tabCtrl;
    CFilterPage m_messagePage;
    CFilterPage m_processPage;
    CRegExDlg m_regExDlg;
    SIZE m_border;
    std::wstring m_name;
    LogFilter m_filter;
};

} // namespace debugviewpp
} // namespace fusion
