// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>

#pragma warning(push, 3)
#include "PropertyGrid.h"
#pragma warning(pop)
#include "PropertyColorItem.h"
#include "FilterType.h"

namespace fusion {
namespace debugviewpp {

std::wstring GetGridItemText(const CPropertyGridCtrl& grid, int iItem, int iSubItem);

template <typename ItemType>
ItemType& GetGridItem(const CPropertyGridCtrl& grid, int iItem, int iSubItem)
{
	return dynamic_cast<ItemType&>(*grid.GetProperty(iItem, iSubItem));
}

template <size_t N>
CPropertyListItem* CreateFilterTypeItem(const wchar_t* name, const FilterType::type (&types)[N], FilterType::type value)
{
	const wchar_t* items[N + 1];
	int index = 0;
	for (size_t i = 0; i < N; ++i)
	{
		items[i] = FilterTypeToWCharPtr(types[i]);
		if (types[i] == value)
			index = i;
	}
	items[N] = nullptr;
	auto pItem = PropCreateList(name, items);
	pItem->SetValue(CComVariant(index));
	return pItem;
}

} // namespace debugviewpp 
} // namespace fusion
