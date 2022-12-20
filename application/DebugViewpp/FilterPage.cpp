// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "FilterPage.h"

#include "CobaltFusion/AtlWinExt.h"
#include "CobaltFusion/scope_guard.h"
#include "CobaltFusion/fusionassert.h"
#include "CobaltFusion/Str.h"
#include "Win32/Utilities.h"
#include "resource.h"

#include "CommCtrl.h"

#include <memory>

namespace fusion {
namespace debugviewpp {

namespace SubItem {

const int Text = 0;
const int Enable = 1;
const int Match = 2;
const int Type = 3;
const int Background = 4;
const int Foreground = 5;
const int Remove = 6;

} // namespace SubItem

template <typename T>
size_t GetArrayIndex(T value, const T a[], size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        if (a[i] == value)
        {
            return i;
        }
    }
    return 0;
}

bool SupportsColor(FilterType::type filterType)
{
    switch (filterType)
    {
    case FilterType::Exclude:
        return false;
    default:
        return true;
    }
}

CFilterPageImpl::CFilterPageImpl(const FilterType::type* filterTypes, size_t filterTypeCount, const MatchType::type* matchTypes, size_t matchTypeCount, bool supportAutoBg) :
    m_filterTypes(filterTypes),
    m_filterTypeCount(filterTypeCount),
    m_matchTypes(matchTypes),
    m_matchTypeCount(matchTypeCount),
    m_supportAutoBg(supportAutoBg),
    m_dragItem(0),
    m_preResizeWidth(0)
{
}

BEGIN_MSG_MAP2(CFilterPageImpl)
    MSG_WM_INITDIALOG(OnInitDialog)
    MSG_WM_DESTROY(OnDestroy)
    MSG_WM_MOUSEMOVE(OnMouseMove)
    MSG_WM_LBUTTONUP(OnLButtonUp)
    MSG_WM_SIZE(OnSize)
    NOTIFY_CODE_HANDLER_EX(HDN_ITEMSTATEICONCLICK, OnHeaderItemStateIconClick)
    NOTIFY_CODE_HANDLER_EX(LVN_BEGINDRAG, OnDrag)
    NOTIFY_CODE_HANDLER_EX(PIN_ADDITEM, OnAddItem)
    NOTIFY_CODE_HANDLER_EX(PIN_CLICK, OnClickItem)
    NOTIFY_CODE_HANDLER_EX(PIN_ITEMCHANGED, OnItemChanged)
    REFLECT_NOTIFICATIONS()
    CHAIN_MSG_MAP(CDialogResize<CFilterPageImpl>)
END_MSG_MAP()

void CFilterPageImpl::OnException() const
{
    FUSION_REPORT_EXCEPTION("Unknown Exception");
}

void CFilterPageImpl::OnException(const std::exception& ex) const
{
    FUSION_REPORT_EXCEPTION(ex.what());
}

void CFilterPageImpl::ShowError()
{
    m_grid.SetFocus();
}

bool CFilterPageImpl::SupportsAutoColor(FilterType::type filterType) const
{
    switch (filterType)
    {
    case FilterType::Include:
    case FilterType::Highlight:
    case FilterType::Track:
    case FilterType::Stop:
    case FilterType::Token:
        return m_supportAutoBg;
    default:
        return false;
    }
}

void CFilterPageImpl::UpdateGridColors(int item) const
{
    auto& text = dynamic_cast<CPropertyEditItem&>(*m_grid.GetProperty(item, SubItem::Text));
    auto& bg = dynamic_cast<CPropertyColorItem&>(*m_grid.GetProperty(item, SubItem::Background));
    auto& fg = dynamic_cast<CPropertyColorItem&>(*m_grid.GetProperty(item, SubItem::Foreground));

    auto supportsColor = SupportsColor(GetFilterType(item));
    auto bgColor = bg.GetColor();
    auto autoCol = bgColor == Colors::Auto;
    if (autoCol && !SupportsAutoColor(GetFilterType(item)))
    {
        bg.SetColor(Colors::BackGround);
        autoCol = false;
    }
    text.SetBkColor(supportsColor && !autoCol ? bgColor : Colors::BackGround);
    text.SetTextColor(supportsColor && !autoCol ? fg.GetColor() : Colors::Text);

    bg.SetEnabled(static_cast<BOOL>(supportsColor));
    bg.ShowAuto(SupportsAutoColor(GetFilterType(item)));

    fg.SetEnabled(static_cast<BOOL>(supportsColor && !autoCol));
}

void CFilterPageImpl::InsertFilter(int item, const Filter& filter)
{
    auto pFilterProp = PropCreateSimple(L"", WStr(filter.text));
    auto pBkColor = PropCreateColorItem(L"Background Color", filter.bgColor);
    auto pTxColor = PropCreateColorItem(L"Text Color", filter.fgColor);
    auto pFilter = CreateEnumTypeItem(L"", m_filterTypes, m_filterTypeCount, filter.filterType);
    pFilter->SetEnabled(static_cast<BOOL>(filter.matchType != MatchType::RegexGroups));

    m_grid.InsertItem(item, pFilterProp);
    m_grid.SetSubItem(item, SubItem::Enable, PropCreateCheckButton(L"", filter.enable));
    m_grid.SetSubItem(item, SubItem::Match, CreateEnumTypeItem(L"", m_matchTypes, m_matchTypeCount, filter.matchType));
    m_grid.SetSubItem(item, SubItem::Type, pFilter);
    m_grid.SetSubItem(item, SubItem::Background, pBkColor);
    m_grid.SetSubItem(item, SubItem::Foreground, pTxColor);
    m_grid.SetSubItem(item, SubItem::Remove, PropCreateReadOnlyItem(L"", L"x"));
    UpdateGridColors(item);
    m_grid.SelectItem(item);
}

void CFilterPageImpl::AddFilter(const Filter& filter)
{
    InsertFilter(m_grid.GetItemCount(), filter);
}

BOOL CFilterPageImpl::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    m_grid.SubclassWindow(GetDlgItem(IDC_FILTER_GRID));
    m_grid.InsertColumn(SubItem::Enable, L"", LVCFMT_LEFT, 24, 0, -1, 0);
    m_grid.InsertColumn(SubItem::Text, L"Filter", LVCFMT_LEFT, 174, 0, -1, 1);
    m_grid.InsertColumn(SubItem::Match, L"Match", LVCFMT_LEFT, 76, 0, -1, 2);
    m_grid.InsertColumn(SubItem::Type, L"Type", LVCFMT_LEFT, 55, 0, -1, 3);
    m_grid.InsertColumn(SubItem::Background, L"Bg", LVCFMT_LEFT, 24, 0, -1, 4);
    m_grid.InsertColumn(SubItem::Foreground, L"Fg", LVCFMT_LEFT, 24, 0, -1, 5);
    m_grid.InsertColumn(SubItem::Remove, L"", LVCFMT_LEFT, 16, 0, -1, 6);
    m_grid.SetExtendedGridStyle(PGS_EX_SINGLECLICKEDIT | PGS_EX_ADDITEMATEND);

    UpdateGrid();
    RECT rect;
    m_grid.GetWindowRect(&rect);
    m_preResizeWidth = rect.right - rect.left;
    DlgResize_Init(false);

    // focus last item, 1) so mouse-wheel scrolling works 2) because if there are many filters, mostly likely the user wants to edit a recently added filter
    auto lastItem = m_grid.GetItemCount() - 1;
    m_grid.SelectItem(lastItem);

    // Here's where we can add the checkbox to the column header
    // First, we need to snag the header control and give it the
    // HDS_CHECKBOXES style since the list view doesn't do this for us
    auto header = m_grid.GetHeader();
    header.SetWindowLong(GWL_STYLE, header.GetStyle() | HDS_CHECKBOXES);

    // Now, we can update the format for the first header item,
    // which corresponds to the first column
    HDITEM hdi = {0};
    hdi.mask = HDI_FORMAT;
    header.GetItem(SubItem::Enable, &hdi);
    hdi.fmt |= HDF_CHECKBOX | HDF_FIXEDWIDTH;
    header.SetItem(SubItem::Enable, &hdi);
    SetHeaderCheckbox();

    return TRUE;
}

void CFilterPageImpl::OnDestroy()
{
}

void CFilterPageImpl::CheckAllItems(bool checked)
{
    int count = m_grid.GetItemCount();
    for (int i = 0; i < count; ++i)
    {
        SetFilterEnable(i, checked);
    }
    m_grid.Invalidate();
}

void CFilterPageImpl::SetHeaderCheckbox()
{
    bool checked = true;
    int count = m_grid.GetItemCount();
    for (int i = 0; i < count; ++i)
    {
        if (!GetFilterEnable(i))
        {
            checked = false;
            break;
        }
    }

    auto header = m_grid.GetHeader();
    HDITEM hdi = {0};
    hdi.mask = HDI_FORMAT;
    header.GetItem(SubItem::Enable, &hdi);
    if (checked)
    {
        hdi.fmt |= HDF_CHECKED;
    }
    else
    {
        hdi.fmt &= ~HDF_CHECKED;
    }
    header.SetItem(SubItem::Enable, &hdi);
    header.Invalidate();
}

LRESULT CFilterPageImpl::OnHeaderItemStateIconClick(NMHDR* phdr)
{
    auto& nmHeader = *reinterpret_cast<NMHEADER*>(phdr);

    if (((nmHeader.pitem->mask & HDI_FORMAT) != 0u) && ((nmHeader.pitem->fmt & HDF_CHECKBOX) != 0))
    {
        CheckAllItems((nmHeader.pitem->fmt & HDF_CHECKED) == 0);
        SetHeaderCheckbox();
        return 1;
    }

    return 0;
}

LRESULT CFilterPageImpl::OnDrag(NMHDR* phdr)
{
    NMLISTVIEW& lv = *reinterpret_cast<NMLISTVIEW*>(phdr);

    if (lv.iItem < 0 || lv.iItem >= static_cast<int>(m_filters.size()))
    {
        return 0;
    }

    m_dragItem = lv.iItem;
    POINT pt = {0};
    m_dragImage = m_grid.CreateDragImage(lv.iItem, &pt);

    RECT rect = {0};
    m_grid.GetItemRect(lv.iItem, &rect, LVIR_BOUNDS);
    m_dragImage.BeginDrag(0, lv.ptAction.x - rect.left, lv.ptAction.y - rect.top);
    WTL::CImageList::DragEnter(*this, lv.ptAction);
    SetCapture();
    m_dragCursor = std::make_unique<Win32::ScopedCursor>(LoadCursor(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDC_DRAGDROP)));
    return 0;
}

void CFilterPageImpl::OnMouseMove(UINT /*nFlags*/, CPoint point) const
{
    if (!m_dragImage.IsNull())
    {
        WTL::CImageList::DragMove(point);
    }
}

void CFilterPageImpl::OnLButtonUp(UINT /*nFlags*/, CPoint point)
{
    if (!m_dragImage.IsNull())
    {
        WTL::CImageList::DragLeave(*this);
        WTL::CImageList::EndDrag();
        ReleaseCapture();
        m_dragImage.Destroy();
        m_dragCursor.reset();

        UINT flags = 0;
        std::size_t item = std::min<size_t>(m_grid.HitTest(point, &flags), m_filters.size() - 1);
        if (item < m_filters.size())
        {
            auto filter = GetFilter(m_dragItem);
            m_grid.DeleteItem(m_dragItem);
            InsertFilter(static_cast<int>(item), filter);
        }
    }
}

void CFilterPageImpl::OnSize(UINT /*type*/, CSize size)
{
    m_grid.SetColumnWidth(SubItem::Text, m_grid.GetColumnWidth(SubItem::Text) + size.cx - m_preResizeWidth);
    m_preResizeWidth = size.cx;
    SetMsgHandled(win32::False);
}

LRESULT CFilterPageImpl::OnAddItem(NMHDR* /*pnmh*/)
{
    Filter filter;
    AddFilter(filter);
    m_filters.push_back(filter);
    m_grid.SendMessage(WM_KEYDOWN, VK_F2, 0);

    return 0;
}

LRESULT CFilterPageImpl::OnClickItem(NMHDR* pnmh)
{
    auto& nmhdr = *reinterpret_cast<NMPROPERTYITEM*>(pnmh);

    int iItem;
    int iSubItem;
    if ((m_grid.FindProperty(nmhdr.prop, iItem, iSubItem) != 0) && iSubItem == SubItem::Remove)
    {
        m_grid.DeleteItem(iItem);
        m_filters.erase(m_filters.begin() + iItem);
        return TRUE;
    }

    return FALSE;
}

LRESULT CFilterPageImpl::OnItemChanged(NMHDR* pnmh)
{
    auto& nmhdr = *reinterpret_cast<NMPROPERTYITEM*>(pnmh);

    int iItem;
    int iSubItem;
    if (m_grid.FindProperty(nmhdr.prop, iItem, iSubItem) == 0)
    {
        return FALSE;
    }

    if (iSubItem == SubItem::Match)
    {
        auto& type = dynamic_cast<CPropertyListItem&>(*m_grid.GetProperty(iItem, SubItem::Type));
        if (GetMatchType(iItem) == MatchType::RegexGroups)
        {
            type.SetValue(CComVariant(GetArrayIndex(FilterType::Token, m_filterTypes, m_filterTypeCount)));
            type.SetEnabled(win32::False);
        }
        else
        {
            type.SetEnabled(win32::True);
        }
    }

    if (iSubItem == SubItem::Type)
    {
        UpdateGridColors(iItem);
    }

    if (iSubItem == SubItem::Background)
    {
        UpdateGridColors(iItem);
    }

    if (iSubItem == SubItem::Foreground)
    {
        UpdateGridColors(iItem);
    }

    if (iSubItem == SubItem::Enable)
    {
        SetHeaderCheckbox();
    }

    return 0;
}

bool CFilterPageImpl::GetFilterEnable(int iItem) const
{
    CComVariant val;
    GetGridItem<CPropertyCheckButtonItem>(m_grid, iItem, SubItem::Enable).GetValue(&val);
    return val.boolVal != VARIANT_FALSE;
}

void CFilterPageImpl::SetFilterEnable(int iItem, bool value) const
{
    CComVariant val(value);
    GetGridItem<CPropertyCheckButtonItem>(m_grid, iItem, SubItem::Enable).SetValue(val);
}

std::wstring CFilterPageImpl::GetFilterText(int iItem) const
{
    return GetGridItemText(m_grid, iItem, SubItem::Text);
}

MatchType::type CFilterPageImpl::GetMatchType(int iItem) const
{
    CComVariant val;
    GetGridItem<CPropertyListItem>(m_grid, iItem, SubItem::Match).GetValue(&val);
    return m_matchTypes[val.lVal];
}

FilterType::type CFilterPageImpl::GetFilterType(int iItem) const
{
    CComVariant val;
    GetGridItem<CPropertyListItem>(m_grid, iItem, SubItem::Type).GetValue(&val);
    return m_filterTypes[val.lVal];
}

COLORREF CFilterPageImpl::GetFilterBgColor(int iItem) const
{
    CComVariant val;
    GetGridItem<CPropertyColorItem>(m_grid, iItem, SubItem::Background).GetValue(&val);
    return val.lVal;
}

COLORREF CFilterPageImpl::GetFilterFgColor(int iItem) const
{
    CComVariant val;
    GetGridItem<CPropertyColorItem>(m_grid, iItem, SubItem::Foreground).GetValue(&val);
    return val.lVal;
}

Filter CFilterPageImpl::GetFilter(int item) const
{
    return Filter(Str(GetFilterText(item)), GetMatchType(item), GetFilterType(item), GetFilterBgColor(item), GetFilterFgColor(item), GetFilterEnable(item));
}

std::vector<Filter> CFilterPageImpl::GetFilters()
{
    std::vector<Filter> filters;
    int n = m_grid.GetItemCount();
    filters.reserve(n);

    for (int i = 0; i < n; ++i)
    {
        m_grid.SetFocus();
        m_grid.SelectItem(i);
        m_grid.SendMessage(WM_KEYDOWN, VK_F2, 0);
        filters.emplace_back(Str(GetFilterText(i)), GetMatchType(i), GetFilterType(i), GetFilterBgColor(i), GetFilterFgColor(i), GetFilterEnable(i));
    }

    return filters;
}

void CFilterPageImpl::SetFilters(const std::vector<Filter>& filters)
{
    m_filters = filters;
    if (IsWindow() != 0)
    {
        UpdateGrid();
    }
}

void CFilterPageImpl::UpdateGrid(int focus)
{
    m_grid.DeleteAllItems();
    for (auto& filter : m_filters)
    {
        AddFilter(filter);
    }
    m_grid.SelectItem(focus);
}

} // namespace debugviewpp
} // namespace fusion
