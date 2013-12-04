#pragma once

#include <map>

namespace gj {

class DisplayInfo 
{
public:
	DisplayInfo();
	void Clear();

	std::wstring GetProcessName(DWORD pid);

private:
	std::map<DWORD, std::wstring> m_processNames;
};

} // namespace gj
