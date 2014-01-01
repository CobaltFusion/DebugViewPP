// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "ProcessFilterPage.h"
#pragma warning(push, 3)
#include "PropertyColorItem.h"
#pragma warning(pop)

namespace fusion {
namespace debugviewpp {

ProcessFilter::ProcessFilter() :
	pid(0),
	type(FilterType::Include),
	bgColor(RGB(255, 255, 255)),
	fgColor(RGB(  0,   0,   0)),
	enable(true)
{
}

ProcessFilter::ProcessFilter(const std::string& text, DWORD pid, FilterType::type type, COLORREF bgColor, COLORREF fgColor, bool enable) :
	text(text), re(text, std::regex_constants::icase | std::regex_constants::optimize), pid(pid), type(type), bgColor(bgColor), fgColor(fgColor), enable(enable)
{
}

CProcessFilterPage::CProcessFilterPage(const std::vector<ProcessFilter>& filters) :
	m_filters(filters)
{
}

BEGIN_MSG_MAP_TRY(CProcessFilterPage)
	MSG_WM_INITDIALOG(OnInitDialog)
	MSG_WM_DESTROY(OnDestroy)
	NOTIFY_CODE_HANDLER_EX(PIN_ADDITEM, OnAddItem)
	NOTIFY_CODE_HANDLER_EX(PIN_CLICK, OnClickItem)
	NOTIFY_CODE_HANDLER_EX(PIN_ITEMCHANGED, OnItemChanged)
	REFLECT_NOTIFICATIONS()
	CHAIN_MSG_MAP(CDialogResize<CProcessFilterPage>)
END_MSG_MAP_CATCH(ExceptionHandler)

void CProcessFilterPage::ExceptionHandler()
{
	MessageBox(WStr(GetExceptionMessage()), LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
}

void CProcessFilterPage::AddFilter(const ProcessFilter& filter)
{
	int item = m_grid.GetItemCount();
	m_grid.InsertItem(item, PropCreateCheckButton(L"", filter.enable));

	static const wchar_t* types[] = { L"Include", L"Exclude", L"Highlight", nullptr };
	auto pTypeList = PropCreateList(L"", types);
	pTypeList->SetValue(CComVariant(filter.type));
	auto pFilterProp = PropCreateSimple(L"", WStr(filter.text));
	pFilterProp->SetBkColor(filter.bgColor);
	pFilterProp->SetTextColor(filter.fgColor);
	m_grid.SetSubItem(item, 1, pFilterProp);
	m_grid.SetSubItem(item, 2, pTypeList);
	m_grid.SetSubItem(item, 3, PropCreateColorItem(L"Background Color", filter.bgColor));
	m_grid.SetSubItem(item, 4, PropCreateColorItem(L"Text Color", filter.fgColor));
	m_grid.SetSubItem(item, 5, PropCreateReadOnlyItem(L"", L"×"));
	m_grid.SelectItem(item);
}

BOOL CProcessFilterPage::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
	m_grid.SubclassWindow(GetDlgItem(IDC_GRID));
	m_grid.InsertColumn(0, L"", LVCFMT_LEFT, 32, 0);
	m_grid.InsertColumn(1, L"Filter", LVCFMT_LEFT, 200, 0);
	m_grid.InsertColumn(2, L"Type", LVCFMT_LEFT, 60, 0);
	m_grid.InsertColumn(3, L"Bg", LVCFMT_LEFT, 20, 0);
	m_grid.InsertColumn(4, L"Fg", LVCFMT_LEFT, 20, 0);
	m_grid.InsertColumn(5, L"", LVCFMT_LEFT, 16, 0);
	m_grid.SetExtendedGridStyle(PGS_EX_SINGLECLICKEDIT | PGS_EX_ADDITEMATEND);

	for (auto it = m_filters.begin(); it != m_filters.end(); ++it)
		AddFilter(*it);

	DlgResize_Init(false);

	return TRUE;
}

void CProcessFilterPage::OnDestroy()
{
}

LRESULT CProcessFilterPage::OnAddItem(NMHDR* /*pnmh*/)
{
	AddFilter(ProcessFilter());
	return 0;
}

LRESULT CProcessFilterPage::OnClickItem(NMHDR* pnmh)
{
	auto pClick = reinterpret_cast<NMPROPERTYITEM*>(pnmh);

	int iItem;
	int iSubItem;
	if (m_grid.FindProperty(pClick->prop, iItem, iSubItem) && iSubItem == 5)
	{
		m_grid.DeleteItem(iItem);
		return TRUE;
	}

	return FALSE;
}

LRESULT CProcessFilterPage::OnItemChanged(NMHDR* pnmh)
{
	auto pItemChanged = reinterpret_cast<NMPROPERTYITEM*>(pnmh);

	int iItem;
	int iSubItem;
	if (!m_grid.FindProperty(pItemChanged->prop, iItem, iSubItem))
		return FALSE;
	
	if (iSubItem == 3)
	{
		CPropertyColorItem& item = dynamic_cast<CPropertyColorItem&>(*pItemChanged->prop);
		CPropertyEditItem& edit = dynamic_cast<CPropertyEditItem&>(*m_grid.GetProperty(iItem, 1));
		edit.SetBkColor(item.GetColor());
		return TRUE;
	}

	if (iSubItem == 4)
	{
		CPropertyColorItem& item = dynamic_cast<CPropertyColorItem&>(*pItemChanged->prop);
		CPropertyEditItem& edit = dynamic_cast<CPropertyEditItem&>(*m_grid.GetProperty(iItem, 1));
		edit.SetTextColor(item.GetColor());
		return TRUE;
	}

	return 0;
}

bool CProcessFilterPage::GetFilterEnable(int iItem) const
{
	CComVariant val;
	GetGridItem<CPropertyCheckButtonItem>(m_grid, iItem, 0).GetValue(&val);
	return val.boolVal != VARIANT_FALSE;
}

std::wstring CProcessFilterPage::GetFilterText(int iItem) const
{
	return GetGridItemText(m_grid, iItem, 1);
}

FilterType::type CProcessFilterPage::GetFilterType(int iItem) const
{
	CComVariant val;
	GetGridItem<CPropertyListItem>(m_grid, iItem, 2).GetValue(&val);
	return IntToFilterType(val.lVal);
}

COLORREF CProcessFilterPage::GetFilterBgColor(int iItem) const
{
	CComVariant val;
	GetGridItem<CPropertyColorItem>(m_grid, iItem, 3).GetValue(&val);
	return val.lVal;
}

COLORREF CProcessFilterPage::GetFilterFgColor(int iItem) const
{
	CComVariant val;
	GetGridItem<CPropertyColorItem>(m_grid, iItem, 4).GetValue(&val);
	return val.lVal;
}

std::vector<ProcessFilter> CProcessFilterPage::GetFilters() const
{
	std::vector<ProcessFilter> filters;
	int n = m_grid.GetItemCount();
	filters.reserve(n);

	for (int i = 0; i < n; ++i)
		filters.push_back(ProcessFilter(Str(GetFilterText(i)), 0, GetFilterType(i), GetFilterBgColor(i), GetFilterFgColor(i), GetFilterEnable(i)));

	return filters;
}

void CProcessFilterPage::SetFilters(const std::vector<ProcessFilter>& filters)
{
	m_grid.DeleteAllItems();
	for (auto it = filters.begin(); it != filters.end(); ++it)
		AddFilter(*it);
}

} // namespace debugviewpp 
} // namespace fusion
