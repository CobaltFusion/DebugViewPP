// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <vector>
#include "CobaltFusion/AtlWinExt.h"
#include "DebugViewppLib/LogSources.h"
#include "Grid.h"
#include "PropertyColorItem.h"
#include "resource.h"
#include "Filter.h"

namespace fusion {
namespace debugviewpp {

class CSourcesDlg : public CDialogImpl<CSourcesDlg>,
                    public CDialogResize<CSourcesDlg>,
                    public ExceptionHandler<CSourcesDlg, std::exception>
{
public:
    explicit CSourcesDlg(std::vector<SourceInfo> sourceInfos);

    enum
    {
        IDD = IDD_SOURCES
    };

    BEGIN_DLGRESIZE_MAP(CSourcesDlg)
        DLGRESIZE_CONTROL(IDADD, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_SOURCES_GRID, DLSZ_SIZE_X | DLSZ_SIZE_Y)
    END_DLGRESIZE_MAP()

    std::vector<SourceInfo> GetSourceInfos();

private:
    DECLARE_MSG_MAP()

    void OnException();
    void OnException(const std::exception& ex);
    BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
    void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnOk(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnAdd(UINT uNotifyCode, int nID, CWindow wndCtl);
    LRESULT OnClickItem(NMHDR* pnmh);

    bool GetSourceEnable(int iItem) const;
    std::wstring GetSourceText(int iItem) const;
    SourceType::type GetSourceType(int iItem) const;
    void UpdateGrid();

    CPropertyGridCtrl m_grid;
    std::vector<SourceInfo> m_sourceInfos;
};

} // namespace debugviewpp
} // namespace fusion
