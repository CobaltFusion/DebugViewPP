// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Grid.h"

namespace fusion {
namespace debugviewpp {

std::wstring GetGridItemText(const CPropertyGridCtrl& grid, int iItem, int iSubItem)
{
	const int BufSize = 4192;
	wchar_t buf[BufSize];
	if (grid.GetItemText(iItem, iSubItem, buf, BufSize))
		return buf;
	return L"";
}

} // namespace debugviewpp 
} // namespace fusion
