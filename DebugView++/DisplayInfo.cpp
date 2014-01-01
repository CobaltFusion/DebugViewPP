// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <map>
#include "DisplayInfo.h"
#include "ProcessInfo.h"

namespace fusion {
namespace debugviewpp {

void DisplayInfo::Clear()
{
	m_processNames.clear();
}

std::wstring DisplayInfo::GetProcessName(DWORD pid /* , timestamp */) const
{
	// todo: record processid->name relationship by last-seen timestamp and refresh it after 1 minute?
	auto entry = m_processNames.find(pid);
	if (m_processNames.end() != entry)
		return entry->second;

	auto name = ProcessInfo::GetProcessName(pid);
	m_processNames[pid] = name;
	return name;
}

} // namespace debugviewpp 
} // namespace fusion
