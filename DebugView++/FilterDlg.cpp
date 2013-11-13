//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://boosttestui.wordpress.com/ for the boosttestui home page.

#include "stdafx.h"
#include "atlstr.h"
#include "resource.h"
#include "Utilities.h"
#include "FilterDlg.h"

namespace gj {

BEGIN_MSG_MAP_TRY(CFilterDlg)
	MSG_WM_INITDIALOG(OnInitDialog)
	MSG_WM_DESTROY(OnDestroy)
	COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
	COMMAND_ID_HANDLER_EX(IDOK, OnOk)
	NOTIFY_CODE_HANDLER_EX(PIN_ADDITEM, OnAddItem);
	NOTIFY_CODE_HANDLER_EX(PIN_CLICK, OnClickItem);
	REFLECT_NOTIFICATIONS()
	CHAIN_MSG_MAP(CDialogResize<CFilterDlg>)
END_MSG_MAP_CATCH(ExceptionHandler)

CFilterDlg::CFilterDlg(const std::wstring& name) :
	m_name(name)
{
}

CFilterDlg::CFilterDlg(const std::wstring& name, const std::vector<std::string>& filters) :
	m_name(name),
	m_filters(filters)
{
}

std::wstring CFilterDlg::GetName() const
{
	return m_name;
}

std::vector<std::string> CFilterDlg::GetFilters() const
{
	return m_filters;
}

void CFilterDlg::ExceptionHandler()
{
	MessageBox(WStr(GetExceptionMessage()), LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
}

std::unique_ptr<CColorPickerListCtrl> CFilterDlg::CreateColorCtrl()
{
	auto pCtrl = make_unique<CColorPickerListCtrl>();
	pCtrl->Create(*this, rcDefault, nullptr, WS_CHILD | WS_BORDER);
	pCtrl->SetParent(NULL);
	pCtrl->AddColor(0, RGB(255,255,255));
	pCtrl->AddColor(1, RGB(255,0,0));
	pCtrl->AddColor(2, RGB(0,255,0));
	pCtrl->AddColor(3, RGB(0,0,255));
	return pCtrl;
}

void CFilterDlg::AddFilter(const std::wstring& filter)
{
	int item = m_grid.GetItemCount();
	m_grid.InsertItem(item, PropCreateSimple(L"", filter.c_str()));
	const wchar_t* types[] = { L"Exclude", L"Include", L"Highlight" , nullptr };
	m_grid.SetSubItem(item, 1, PropCreateList(L"", types));
	m_colorCtrls.push_back(CreateColorCtrl());
	m_grid.SetSubItem(item, 2, PropCreateComboControl(L"", *m_colorCtrls.back(), RGB(0,255,0)));
	m_grid.SetSubItem(item, 3, PropCreateCheckButton(L"", true));
	m_grid.SetSubItem(item, 4, PropCreateReadOnlyItem(L"", L"×"));
	m_grid.SelectItem(item);
}

void CFilterDlg::RemoveFilter(int index)
{
	m_grid.DeleteItem(index);
	auto it = m_colorCtrls.begin() + index;
	(*it)->DestroyWindow();
	m_colorCtrls.erase(it);
}

BOOL CFilterDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
	SetDlgItemText(IDC_NAME, m_name.c_str());

	m_grid.SubclassWindow(GetDlgItem(IDC_GRID));
	m_grid.InsertColumn(0, L"Filter", LVCFMT_LEFT, 200, 0);
	m_grid.InsertColumn(1, L"Type", LVCFMT_LEFT, 60, 0);
	m_grid.InsertColumn(2, L"Highlight", LVCFMT_LEFT, 90, 0);
	m_grid.InsertColumn(3, L"", LVCFMT_LEFT, 32, 0);
	m_grid.InsertColumn(4, L"", LVCFMT_LEFT, 16, 0);
	m_grid.SetExtendedGridStyle(PGS_EX_SINGLECLICKEDIT | PGS_EX_ADDITEMATEND);

	for (auto it = m_filters.begin(); it != m_filters.end(); ++it)
		AddFilter(WStr(*it));

	CenterWindow(GetParent());
	DlgResize_Init();
	return TRUE;
}

void CFilterDlg::OnDestroy()
{
	for (auto it = m_colorCtrls.begin(); it != m_colorCtrls.end(); ++it)
		(*it)->DestroyWindow();
}

LRESULT CFilterDlg::OnAddItem(NMHDR* pnmh)
{
	AddFilter(L"");
	return 0;
}

LRESULT CFilterDlg::OnClickItem(NMHDR* pnmh)
{
	auto pClick = reinterpret_cast<NMPROPERTYITEM*>(pnmh);

	int iItem;
	int iSubItem;
	if (m_grid.FindProperty(pClick->prop, iItem, iSubItem) && iSubItem == 4)
	{
		RemoveFilter(iItem);
		return TRUE;
	}

	return FALSE;
}

void CFilterDlg::OnCancel(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
	EndDialog(nID);
}

std::wstring GetGridItemText(const CPropertyGridCtrl& grid, int iItem, int iSubItem)
{
	const int BufSize = 1024;
	wchar_t buf[BufSize];
	if (grid.GetItemText(iItem, iSubItem, buf, BufSize))
		return buf;
	return L"";
}

void CFilterDlg::OnOk(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
	m_name = gj::GetDlgItemText(*this, IDC_NAME);

	int n = m_grid.GetItemCount();
	for (int i = 0; i < n; ++i)
		m_filters.push_back(Str(GetGridItemText(m_grid, i, 0)));

	EndDialog(nID);
}

} // namespace gj
