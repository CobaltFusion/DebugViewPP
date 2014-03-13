// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Win32Lib/utilities.h"
#include "Resource.h"
#include "FilterPage.h"

namespace fusion {
namespace debugviewpp {

CFilterPageImpl::CFilterPageImpl(const FilterType::type* filterTypes, size_t filterTypeCount, const MatchType::type* matchTypes, size_t matchTypeCount) :
	m_filterTypes(filterTypes), m_filterTypeCount(filterTypeCount),
	m_matchTypes(matchTypes), m_matchTypeCount(matchTypeCount)
{
}

BEGIN_MSG_MAP_TRY(CFilterPageImpl)
	MSG_WM_INITDIALOG(OnInitDialog)
	MSG_WM_DESTROY(OnDestroy)
	NOTIFY_CODE_HANDLER_EX(PIN_ADDITEM, OnAddItem)
	NOTIFY_CODE_HANDLER_EX(PIN_CLICK, OnClickItem)
	NOTIFY_CODE_HANDLER_EX(PIN_ITEMCHANGED, OnItemChanged)
	REFLECT_NOTIFICATIONS()
	CHAIN_MSG_MAP(CDialogResize<CFilterPageImpl>)
END_MSG_MAP_CATCH(ExceptionHandler)

void CFilterPageImpl::ExceptionHandler()
{
	MessageBox(WStr(GetExceptionMessage()), LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
}

void CFilterPageImpl::AddFilter(const Filter& filter)
{
	auto pFilterProp = PropCreateSimple(L"", WStr(filter.text));
	pFilterProp->SetBkColor(filter.bgColor);
	pFilterProp->SetTextColor(filter.fgColor);

	auto pBkColor = PropCreateColorItem(L"Background Color", filter.bgColor);
	pBkColor->SetEnabled(filter.filterType != FilterType::Exclude);

	auto pTxColor = PropCreateColorItem(L"Text Color", filter.fgColor);
	pTxColor->SetEnabled(filter.filterType != FilterType::Exclude);

	int item = m_grid.GetItemCount();
	m_grid.InsertItem(item, PropCreateCheckButton(L"", filter.enable));
	m_grid.SetSubItem(item, 1, pFilterProp);
	m_grid.SetSubItem(item, 2, CreateEnumTypeItem(L"", m_matchTypes, m_matchTypeCount, filter.matchType));
	m_grid.SetSubItem(item, 3, CreateEnumTypeItem(L"", m_filterTypes, m_filterTypeCount, filter.filterType));
	m_grid.SetSubItem(item, 4, pBkColor);
	m_grid.SetSubItem(item, 5, pTxColor);
	m_grid.SetSubItem(item, 6, PropCreateReadOnlyItem(L"", L"×"));
	m_grid.SelectItem(item);
}

BOOL CFilterPageImpl::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
	m_grid.SubclassWindow(GetDlgItem(IDC_FILTER_GRID));
	m_grid.InsertColumn(0, L"", LVCFMT_LEFT, 32, 0);
	m_grid.InsertColumn(1, L"Filter", LVCFMT_LEFT, 200, 0);
	m_grid.InsertColumn(2, L"Match", LVCFMT_LEFT, 60, 0);
	m_grid.InsertColumn(3, L"Type", LVCFMT_LEFT, 60, 0);
	m_grid.InsertColumn(4, L"Bg", LVCFMT_LEFT, 24, 0);
	m_grid.InsertColumn(5, L"Fg", LVCFMT_LEFT, 24, 0);
	m_grid.InsertColumn(6, L"", LVCFMT_LEFT, 16, 0);
	m_grid.SetExtendedGridStyle(PGS_EX_SINGLECLICKEDIT | PGS_EX_ADDITEMATEND);

	UpdateGrid();
	DlgResize_Init(false);
	return TRUE;
}

void CFilterPageImpl::OnDestroy()
{
}

LRESULT CFilterPageImpl::OnAddItem(NMHDR* /*pnmh*/)
{
	AddFilter(Filter());
	return 0;
}

LRESULT CFilterPageImpl::OnClickItem(NMHDR* pnmh)
{
	auto& nmhdr = *reinterpret_cast<NMPROPERTYITEM*>(pnmh);

	int iItem;
	int iSubItem;
	if (m_grid.FindProperty(nmhdr.prop, iItem, iSubItem) && iSubItem == 6)
	{
		m_grid.DeleteItem(iItem);
		return TRUE;
	}

	return FALSE;
}

LRESULT CFilterPageImpl::OnItemChanged(NMHDR* pnmh)
{
	auto& nmhdr = *reinterpret_cast<NMPROPERTYITEM*>(pnmh);

	int iItem;
	int iSubItem;
	if (!m_grid.FindProperty(nmhdr.prop, iItem, iSubItem))
		return FALSE;
	
	if (iSubItem == 3)
	{
		auto& bkColor = dynamic_cast<CPropertyColorItem&>(*m_grid.GetProperty(iItem, 4));
		auto& txColor = dynamic_cast<CPropertyColorItem&>(*m_grid.GetProperty(iItem, 5));
		bkColor.SetEnabled(GetFilterType(iItem) != FilterType::Exclude);
		txColor.SetEnabled(GetFilterType(iItem) != FilterType::Exclude);
	}

	if (iSubItem == 4)
	{
		auto& color = dynamic_cast<CPropertyColorItem&>(*nmhdr.prop);
		auto& edit = dynamic_cast<CPropertyEditItem&>(*m_grid.GetProperty(iItem, 1));
		edit.SetBkColor(color.GetColor());
		return TRUE;
	}

	if (iSubItem == 5)
	{
		auto& color = dynamic_cast<CPropertyColorItem&>(*nmhdr.prop);
		auto& edit = dynamic_cast<CPropertyEditItem&>(*m_grid.GetProperty(iItem, 1));
		edit.SetTextColor(color.GetColor());
		return TRUE;
	}

	return 0;
}

bool CFilterPageImpl::GetFilterEnable(int iItem) const
{
	CComVariant val;
	GetGridItem<CPropertyCheckButtonItem>(m_grid, iItem, 0).GetValue(&val);
	return val.boolVal != VARIANT_FALSE;
}

std::wstring CFilterPageImpl::GetFilterText(int iItem) const
{
	return GetGridItemText(m_grid, iItem, 1);
}

MatchType::type CFilterPageImpl::GetMatchType(int iItem) const
{
	CComVariant val;
	GetGridItem<CPropertyListItem>(m_grid, iItem, 2).GetValue(&val);
	return m_matchTypes[val.lVal];
}

FilterType::type CFilterPageImpl::GetFilterType(int iItem) const
{
	CComVariant val;
	GetGridItem<CPropertyListItem>(m_grid, iItem, 3).GetValue(&val);
	return m_filterTypes[val.lVal];
}

COLORREF CFilterPageImpl::GetFilterBgColor(int iItem) const
{
	CComVariant val;
	GetGridItem<CPropertyColorItem>(m_grid, iItem, 4).GetValue(&val);
	return val.lVal;
}

COLORREF CFilterPageImpl::GetFilterFgColor(int iItem) const
{
	CComVariant val;
	GetGridItem<CPropertyColorItem>(m_grid, iItem, 5).GetValue(&val);
	return val.lVal;
}

std::vector<Filter> CFilterPageImpl::GetFilters() const
{
	std::vector<Filter> filters;
	int n = m_grid.GetItemCount();
	filters.reserve(n);

	for (int i = 0; i < n; ++i)
		filters.push_back(Filter(Str(GetFilterText(i)), GetMatchType(i), GetFilterType(i), GetFilterBgColor(i), GetFilterFgColor(i), GetFilterEnable(i)));

	return filters;
}

void CFilterPageImpl::SetFilters(const std::vector<Filter>& filters)
{
	m_filters = filters;
	if (IsWindow())
	{
		UpdateGrid();
	}
}

void CFilterPageImpl::UpdateGrid()
{
	m_grid.DeleteAllItems();
	for (auto it = m_filters.begin(); it != m_filters.end(); ++it)
		AddFilter(*it);
}

} // namespace debugviewpp 
} // namespace fusion
