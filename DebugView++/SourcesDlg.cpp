// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <boost/algorithm/string.hpp>
#include <atlstr.h>
#include "Win32Lib/utilities.h"
#include "DebugView++Lib/LogFilter.h"
#include "DebugView++Lib/LogSource.h"
#include "Resource.h"
#include "SourceDlg.h"
#include "SourcesDlg.h"

namespace fusion {
namespace debugviewpp {

BEGIN_MSG_MAP_TRY(CSourcesDlg)
	MSG_WM_INITDIALOG(OnInitDialog)
	NOTIFY_CODE_HANDLER_EX(PIN_CLICK, OnClickItem)
	COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
	COMMAND_ID_HANDLER_EX(IDOK, OnOk)
	COMMAND_ID_HANDLER_EX(IDADD, OnAdd)
	REFLECT_NOTIFICATIONS()
	CHAIN_MSG_MAP(CDialogResize<CSourcesDlg>)
END_MSG_MAP_CATCH(ExceptionHandler)

SourceInfo::SourceInfo(std::wstring name, SourceType::type sourcetype, std::wstring address, int port)
: name(name), sourcetype(sourcetype), address(address), port(port)
{
}

CSourcesDlg::CSourcesDlg(std::vector<std::shared_ptr<LogSource>> logsources) : m_logsources(logsources)
{
}

void CSourcesDlg::ExceptionHandler()
{
	MessageBox(WStr(GetExceptionMessage()).c_str(), LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
}

BOOL CSourcesDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
	m_grid.SubclassWindow(GetDlgItem(IDC_SOURCES_GRID));
	m_grid.InsertColumn(0, L"", LVCFMT_LEFT, 32, 0);
	m_grid.InsertColumn(1, L"Source", LVCFMT_LEFT, 336, 0);
	m_grid.InsertColumn(2, L"Type", LVCFMT_LEFT, 60, 0);
	m_grid.InsertColumn(3, L"", LVCFMT_LEFT, 16, 0);
	m_grid.SetExtendedGridStyle(PGS_EX_SINGLECLICKEDIT);

	for (auto it=m_logsources.begin(); it != m_logsources.end(); ++it)
	{
		auto description = (*it)->GetDescription();
		auto type = (*it)->GetSourceType();
		auto typeName = WStr(SourceTypeToString(type));

		int item = m_grid.GetItemCount();
		m_grid.InsertItem(item, PropCreateCheckButton(L"", true));
		m_grid.SetSubItem(item, 1, PropCreateReadOnlyItem(L"", description.c_str()));
		m_grid.SetSubItem(item, 2, PropCreateReadOnlyItem(L"", typeName.c_str()));
		if (type == SourceType::System)
			m_grid.SetSubItem(item, 3, PropCreateReadOnlyItem(L"", L""));
		else
			m_grid.SetSubItem(item, 3, PropCreateReadOnlyItem(L"", L"×"));
	}

	CenterWindow(GetParent());
	DlgResize_Init();
	return TRUE;
}

LRESULT CSourcesDlg::OnClickItem(NMHDR* pnmh)
{
	auto& nmhdr = *reinterpret_cast<NMPROPERTYITEM*>(pnmh);

	int iItem = -1;
	int iSubItem = -1;
	if (m_grid.FindProperty(nmhdr.prop, iItem, iSubItem) && iSubItem == 3)
	{
		if (GetSourceType(iItem) != SourceType::System)
		{
			m_grid.DeleteItem(iItem);
			auto source = m_logsources[iItem];
			m_logsourcesToRemove.push_back(source);
			m_logsources.erase(m_logsources.begin()+iItem);
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
	m_logsourcesToRemove.clear();
	EndDialog(nID);
}

void CSourcesDlg::OnOk(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
	EndDialog(nID);
}

void CSourcesDlg::OnAdd(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	CSourceDlg dlg;
	if (dlg.DoModal() != IDOK)
		return;
	m_logsourcesToAdd.push_back(SourceInfo(dlg.GetName(), dlg.GetSourceType(), dlg.GetAddress(), dlg.GetPort()));
}

std::vector<std::shared_ptr<LogSource>> CSourcesDlg::GetSourcesToRemove()
{
	return m_logsourcesToRemove;
}

std::vector<SourceInfo> CSourcesDlg::GetSourcesToAdd()
{
	return m_logsourcesToAdd;
}

} // namespace debugviewpp 
} // namespace fusion
