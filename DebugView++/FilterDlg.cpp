// (C) Copyright Gert-Jan de Vos 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// See http://boosttestui.wordpress.com/ for the boosttestui home page.

#include "stdafx.h"
#include "atlstr.h"
#include "resource.h"
#include "Utilities.h"
#include "PropertyColorItem.h"
#include "FilterDlg.h"

namespace gj {

static COLORREF HighlightColors[16] = 
{
	RGB(255, 255, 255), // white
	RGB(192, 192, 192), // light-grey
	RGB(128, 128, 128), // mid-grey
	RGB( 64,  64,  64), // dark-grey
	RGB(  0,   0,   0), // black
	RGB( 27, 161, 226), // blue
	RGB(160,  80,   0), // brown
	RGB( 51, 153,  51), // green
	RGB(162, 193,  57), // lime
	RGB(216,   0, 115), // magenta
	RGB(240, 150,   9), // mango (orange)
	RGB(230, 113, 184), // pink
	RGB(162,   0, 255), // purple
	RGB(229,  20,   0), // red
	RGB(  0, 171, 169), // teal (viridian)
	RGB(255, 255, 255), // white
};

void InitializeCustomColors()
{
	auto colors = ColorDialog::GetCustomColors();
	for (int i = 0; i < 16; ++i)
		colors[i] = HighlightColors[i];
}

bool CustomColorsInitialized = (InitializeCustomColors(), true);

LogFilter::LogFilter() :
	type(FilterType::Include),
	bgColor(RGB(255, 255, 255)),
	fgColor(RGB(  0,   0,   0)),
	enable(true)
{
}

LogFilter::LogFilter(const std::string& text, FilterType::type type, COLORREF bgColor, COLORREF fgColor, bool enable) :
	text(text), re(text, std::regex_constants::icase), type(type), bgColor(bgColor), fgColor(fgColor), enable(enable)
{
}

BEGIN_MSG_MAP_TRY(CFilterDlg)
	MSG_WM_INITDIALOG(OnInitDialog)
	MSG_WM_DESTROY(OnDestroy)
	COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
	COMMAND_ID_HANDLER_EX(IDOK, OnOk)
	NOTIFY_CODE_HANDLER_EX(PIN_ADDITEM, OnAddItem)
	NOTIFY_CODE_HANDLER_EX(PIN_CLICK, OnClickItem)
	NOTIFY_CODE_HANDLER_EX(PIN_ITEMCHANGED, OnItemChanged)
	REFLECT_NOTIFICATIONS()
	CHAIN_MSG_MAP(CDialogResize<CFilterDlg>)
END_MSG_MAP_CATCH(ExceptionHandler)

CFilterDlg::CFilterDlg(const std::wstring& name) :
	m_name(name)
{
}

CFilterDlg::CFilterDlg(const std::wstring& name, const std::vector<LogFilter>& filters) :
	m_name(name),
	m_filters(filters)
{
}

std::wstring CFilterDlg::GetName() const
{
	return m_name;
}

std::vector<LogFilter> CFilterDlg::GetFilters() const
{
	return m_filters;
}

void CFilterDlg::ExceptionHandler()
{
	MessageBox(WStr(GetExceptionMessage()), LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
}

void CFilterDlg::AddFilter(const LogFilter& filter)
{
	int item = m_grid.GetItemCount();
	m_grid.InsertItem(item, PropCreateCheckButton(L"", filter.enable));

	static const wchar_t* types[] = { L"Include", L"Exclude", L"Highlight" , nullptr };
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

BOOL CFilterDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
	SetDlgItemText(IDC_NAME, m_name.c_str());

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

	CenterWindow(GetParent());
	DlgResize_Init();
	return TRUE;
}

void CFilterDlg::OnDestroy()
{
}

LRESULT CFilterDlg::OnAddItem(NMHDR* pnmh)
{
	AddFilter(LogFilter());
	return 0;
}

LRESULT CFilterDlg::OnClickItem(NMHDR* pnmh)
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

LRESULT CFilterDlg::OnItemChanged(NMHDR* pnmh)
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

template <typename ItemType>
ItemType& GetGridItem(const CPropertyGridCtrl& grid, int iItem, int iSubItem)
{
	return dynamic_cast<ItemType&>(*grid.GetProperty(iItem, iSubItem));
}

bool CFilterDlg::GetFilterEnable(int iItem) const
{
	CComVariant val;
	GetGridItem<CPropertyCheckButtonItem>(m_grid, iItem, 0).GetValue(&val);
	return val.boolVal != VARIANT_FALSE;
}

std::wstring CFilterDlg::GetFilterText(int iItem) const
{
	return GetGridItemText(m_grid, iItem, 1);
}

FilterType::type CFilterDlg::GetFilterType(int iItem) const
{
	CComVariant val;
	GetGridItem<CPropertyListItem>(m_grid, iItem, 2).GetValue(&val);
	switch (val.lVal)
	{
	case FilterType::Include: return FilterType::Include;
	case FilterType::Exclude: return FilterType::Exclude;
	case FilterType::Highlight: return FilterType::Highlight;
	}
	throw std::runtime_error("Unknown FilterType");
}

COLORREF CFilterDlg::GetFilterBgColor(int iItem) const
{
	CComVariant val;
	GetGridItem<CPropertyColorItem>(m_grid, iItem, 3).GetValue(&val);
	return val.lVal;
}

COLORREF CFilterDlg::GetFilterFgColor(int iItem) const
{
	CComVariant val;
	GetGridItem<CPropertyColorItem>(m_grid, iItem, 4).GetValue(&val);
	return val.lVal;
}

void CFilterDlg::OnOk(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
	m_name = gj::GetDlgItemText(*this, IDC_NAME);

	std::vector<LogFilter> filters;
	int n = m_grid.GetItemCount();
	filters.reserve(n);

	for (int i = 0; i < n; ++i)
		filters.push_back(LogFilter(Str(GetFilterText(i)), GetFilterType(i), GetFilterBgColor(i), GetFilterFgColor(i), GetFilterEnable(i)));

	m_filters.swap(filters);
	EndDialog(nID);
}

} // namespace gj
