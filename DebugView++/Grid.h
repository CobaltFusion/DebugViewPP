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

namespace fusion {
namespace debugviewpp {

std::wstring GetGridItemText(const CPropertyGridCtrl& grid, int iItem, int iSubItem);

template <typename ItemType>
ItemType& GetGridItem(const CPropertyGridCtrl& grid, int iItem, int iSubItem)
{
	return dynamic_cast<ItemType&>(*grid.GetProperty(iItem, iSubItem));
}

} // namespace debugviewpp 
} // namespace fusion
