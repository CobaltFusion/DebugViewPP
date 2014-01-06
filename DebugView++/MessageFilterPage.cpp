// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Resource.h"
#include "MessageFilterPage.h"

namespace fusion {
namespace debugviewpp {

MessageFilter::MessageFilter() :
	matchType(MatchType::Simple),
	filterType(FilterType::Include),
	bgColor(RGB(255, 255, 255)),
	fgColor(RGB(  0,   0,   0)),
	enable(true)
{
}

MessageFilter::MessageFilter(const std::string& text, MatchType::type matchType, FilterType::type filterType, COLORREF bgColor, COLORREF fgColor, bool enable) :
	text(text), re(MakePattern(matchType, text), std::regex_constants::icase | std::regex_constants::optimize), matchType(matchType), filterType(filterType), bgColor(bgColor), fgColor(fgColor), enable(enable)
{
}

CMessageFilterPage::CMessageFilterPage(const std::vector<MessageFilter>& filters) :
	m_filters(filters)
{
}

BEGIN_MSG_MAP_TRY(CMessageFilterPage)
	MSG_WM_INITDIALOG(OnInitDialog)
	MSG_WM_DESTROY(OnDestroy)
	NOTIFY_CODE_HANDLER_EX(PIN_ADDITEM, OnAddItem)
	NOTIFY_CODE_HANDLER_EX(PIN_CLICK, OnClickItem)
	NOTIFY_CODE_HANDLER_EX(PIN_ITEMCHANGED, OnItemChanged)
	REFLECT_NOTIFICATIONS()
	CHAIN_MSG_MAP(CDialogResize<CMessageFilterPage>)
END_MSG_MAP_CATCH(ExceptionHandler)

void CMessageFilterPage::ExceptionHandler()
{
	MessageBox(WStr(GetExceptionMessage()), LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
}

static const MatchType::type MatchTypes[] =
{
	MatchType::Simple,
	MatchType::Wildcard,
	MatchType::Regex
};

static const FilterType::type FilterTypes[] =
{
	FilterType::Include,
	FilterType::Exclude,
	FilterType::Highlight,
	FilterType::Token,
	FilterType::Stop,
	FilterType::Track,
	FilterType::Ignore
};

void CMessageFilterPage::AddFilter(const MessageFilter& filter)
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
	m_grid.SetSubItem(item, 2, CreateEnumTypeItem(L"", MatchTypes, filter.matchType));
	m_grid.SetSubItem(item, 3, CreateEnumTypeItem(L"", FilterTypes, filter.filterType));
	m_grid.SetSubItem(item, 4, pBkColor);
	m_grid.SetSubItem(item, 5, pTxColor);
	m_grid.SetSubItem(item, 6, PropCreateReadOnlyItem(L"", L"×"));
	m_grid.SelectItem(item);
}

BOOL CMessageFilterPage::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
	m_grid.SubclassWindow(GetDlgItem(IDC_GRID));
	m_grid.InsertColumn(0, L"", LVCFMT_LEFT, 32, 0);
	m_grid.InsertColumn(1, L"Filter", LVCFMT_LEFT, 200, 0);
	m_grid.InsertColumn(2, L"MatchType", LVCFMT_LEFT, 60, 0);
	m_grid.InsertColumn(3, L"FilterType", LVCFMT_LEFT, 60, 0);
	m_grid.InsertColumn(4, L"Bg", LVCFMT_LEFT, 20, 0);
	m_grid.InsertColumn(5, L"Fg", LVCFMT_LEFT, 20, 0);
	m_grid.InsertColumn(6, L"", LVCFMT_LEFT, 16, 0);
	m_grid.SetExtendedGridStyle(PGS_EX_SINGLECLICKEDIT | PGS_EX_ADDITEMATEND);

	for (auto it = m_filters.begin(); it != m_filters.end(); ++it)
		AddFilter(*it);

	DlgResize_Init(false);

	return TRUE;
}

void CMessageFilterPage::OnDestroy()
{
}

LRESULT CMessageFilterPage::OnAddItem(NMHDR* /*pnmh*/)
{
	AddFilter(MessageFilter());
	return 0;
}

LRESULT CMessageFilterPage::OnClickItem(NMHDR* pnmh)
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

LRESULT CMessageFilterPage::OnItemChanged(NMHDR* pnmh)
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

bool CMessageFilterPage::GetFilterEnable(int iItem) const
{
	CComVariant val;
	GetGridItem<CPropertyCheckButtonItem>(m_grid, iItem, 0).GetValue(&val);
	return val.boolVal != VARIANT_FALSE;
}

std::wstring CMessageFilterPage::GetFilterText(int iItem) const
{
	return GetGridItemText(m_grid, iItem, 1);
}

MatchType::type CMessageFilterPage::GetMatchType(int iItem) const
{
	CComVariant val;
	GetGridItem<CPropertyListItem>(m_grid, iItem, 2).GetValue(&val);
	return MatchTypes[val.lVal];
}

FilterType::type CMessageFilterPage::GetFilterType(int iItem) const
{
	CComVariant val;
	GetGridItem<CPropertyListItem>(m_grid, iItem, 3).GetValue(&val);
	return FilterTypes[val.lVal];
}

COLORREF CMessageFilterPage::GetFilterBgColor(int iItem) const
{
	CComVariant val;
	GetGridItem<CPropertyColorItem>(m_grid, iItem, 4).GetValue(&val);
	return val.lVal;
}

COLORREF CMessageFilterPage::GetFilterFgColor(int iItem) const
{
	CComVariant val;
	GetGridItem<CPropertyColorItem>(m_grid, iItem, 5).GetValue(&val);
	return val.lVal;
}

std::vector<MessageFilter> CMessageFilterPage::GetFilters() const
{
	std::vector<MessageFilter> filters;
	int n = m_grid.GetItemCount();
	filters.reserve(n);

	for (int i = 0; i < n; ++i)
		filters.push_back(MessageFilter(Str(GetFilterText(i)), GetMatchType(i), GetFilterType(i), GetFilterBgColor(i), GetFilterFgColor(i), GetFilterEnable(i)));

	return filters;
}

void CMessageFilterPage::SetFilters(const std::vector<MessageFilter>& filters)
{
	m_grid.DeleteAllItems();
	for (auto it = filters.begin(); it != filters.end(); ++it)
		AddFilter(*it);
}

} // namespace debugviewpp 
} // namespace fusion
