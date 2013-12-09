
// (C) Copyright Gert-Jan de Vos 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// See http://boosttestui.wordpress.com/ for the boosttestui home page.

#include "stdafx.h"
#include <map>
#include "DisplayInfo.h"
#include "ProcessInfo.h"

namespace gj {

DisplayInfo::DisplayInfo()
{
}

void DisplayInfo::Clear()
{
	m_processNames.clear();
}

std::wstring DisplayInfo::GetProcessName(DWORD pid /* , timestamp */)
{
	// todo: record processid->name relationship by last-seen timestamp and refresh it after 1 minute?
	auto entry = m_processNames.find(pid);
	if (m_processNames.end() != entry)
	{
		return entry->second;
	}
	auto name = ProcessInfo::GetProcessName(pid);
	m_processNames[pid] = name;
	return name;
}

} // namespace gj
