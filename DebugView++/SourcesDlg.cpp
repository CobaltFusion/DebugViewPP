// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <boost/algorithm/string.hpp>
#include <utility>
#include <atlstr.h>
#include "Win32/Utilities.h"
#include "CobaltFusion/Str.h"
#include "CobaltFusion/AtlWinExt.h"
#include "CobaltFusion/fusionassert.h"
#include "DebugView++Lib/LogFilter.h"
#include "DebugView++Lib/LogSource.h"
#include "resource.h"
#include "SourceDlg.h"
#include "SourcesDlg.h"

namespace fusion {
namespace debugviewpp {

BEGIN_MSG_MAP2(CSourcesDlg)
    MSG_WM_INITDIALOG(OnInitDialog)
    NOTIFY_CODE_HANDLER_EX(PIN_CLICK, OnClickItem)
    COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
    COMMAND_ID_HANDLER_EX(IDOK, OnOk)
    COMMAND_ID_HANDLER_EX(IDADD, OnAdd)
    REFLECT_NOTIFICATIONS()
    CHAIN_MSG_MAP(CDialogResize<CSourcesDlg>)
END_MSG_MAP()

CSourcesDlg::CSourcesDlg(std::vector<SourceInfo> sourceInfos) :
    m_sourceInfos(std::move(sourceInfos))
{
}

void CSourcesDlg::OnException()
{
    FUSION_REPORT_EXCEPTION("Unknown Exception");
}

void CSourcesDlg::OnException(const std::exception& ex)
{
    FUSION_REPORT_EXCEPTION(ex.what());
}

void CSourcesDlg::UpdateGrid()
{
    m_grid.DeleteAllItems();
    for (auto& sourceInfo : m_sourceInfos)
    {
        int item = m_grid.GetItemCount();
        m_grid.InsertItem(item, PropCreateCheckButton(L"", sourceInfo.enabled));
        m_grid.SetSubItem(item, 1, PropCreateReadOnlyItem(L"", sourceInfo.description.c_str()));
        m_grid.SetSubItem(item, 2, PropCreateReadOnlyItem(L"", WStr(SourceTypeToString(sourceInfo.type))));
        if (sourceInfo.type == SourceType::System)
        {
            m_grid.SetSubItem(item, 3, PropCreateReadOnlyItem(L"", L""));
        }
        else
        {
            m_grid.SetSubItem(item, 3, PropCreateReadOnlyItem(L"", L"x"));
        }
    }
}

BOOL CSourcesDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    m_grid.SubclassWindow(GetDlgItem(IDC_SOURCES_GRID));
    m_grid.InsertColumn(0, L"", LVCFMT_LEFT, 32, 0);
    m_grid.InsertColumn(1, L"Source description", LVCFMT_LEFT, 280, 0);
    m_grid.InsertColumn(2, L"Type", LVCFMT_LEFT, 100, 0);
    m_grid.InsertColumn(3, L"", LVCFMT_LEFT, 16, 0);
    m_grid.SetExtendedGridStyle(PGS_EX_SINGLECLICKEDIT);

    UpdateGrid();
    CenterWindow(GetParent());
    DlgResize_Init();
    return TRUE;
}

LRESULT CSourcesDlg::OnClickItem(NMHDR* pnmh)
{
    auto& nmhdr = *reinterpret_cast<NMPROPERTYITEM*>(pnmh);

    int iItem = -1;
    int iSubItem = -1;
    if ((m_grid.FindProperty(nmhdr.prop, iItem, iSubItem) != 0) && iSubItem == 3)
    {
        if (GetSourceType(iItem) != SourceType::System)
        {
            m_grid.DeleteItem(iItem);
            m_sourceInfos.erase(m_sourceInfos.begin() + iItem);
            return TRUE;
        }
    }
    return FALSE;
}

bool CSourcesDlg::GetSourceEnable(int iItem) const
{
    CComVariant val;
    GetGridItem<CPropertyCheckButtonItem>(m_grid, iItem, 0).GetValue(&val);
    return val.boolVal != VARIANT_FALSE;
}

std::wstring CSourcesDlg::GetSourceText(int iItem) const
{
    return GetGridItemText(m_grid, iItem, 1);
}

SourceType::type CSourcesDlg::GetSourceType(int iItem) const
{
    return StringToSourceType(Str(GetGridItemText(m_grid, iItem, 2)));
}

void CSourcesDlg::OnCancel(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
    m_sourceInfos.clear();
    EndDialog(nID);
}

void CSourcesDlg::OnOk(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
    EndDialog(nID);
}

void CSourcesDlg::OnAdd(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CSourceDlg dlg(L"UDP port 2020", SourceType::Udp, L"192.168.1.1", 2020);
    if (dlg.DoModal() != IDOK)
    {
        return;
    }

    auto info = SourceInfo(dlg.GetName(), dlg.GetSourceType(), dlg.GetAddress(), dlg.GetPort());
    info.enabled = true;
    m_sourceInfos.push_back(info);
    UpdateGrid();
}

std::vector<SourceInfo> CSourcesDlg::GetSourceInfos()
{
    return m_sourceInfos;
}

} // namespace debugviewpp
} // namespace fusion
