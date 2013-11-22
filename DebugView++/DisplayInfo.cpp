
#include "stdafx.h"
#include "DisplayInfo.h"
#include "ProcessInfo.h"

#include <map>

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