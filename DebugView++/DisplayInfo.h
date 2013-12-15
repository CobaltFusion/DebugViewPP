// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <map>

namespace fusion {

class DisplayInfo 
{
public:
	DisplayInfo();
	void Clear();

	std::wstring GetProcessName(DWORD pid);

private:
	std::map<DWORD, std::wstring> m_processNames;
};

} // namespace fusion
