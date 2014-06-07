// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include "Win32Lib/Win32Lib.h"
#include "DebugView++Lib/Line.h"
#include "DebugView++Lib/ProcessHandleCache.h"
#include "Win32Lib/utilities.h"

namespace fusion {
namespace debugviewpp {

class NewlineFilter {
public:
	NewlineFilter();
	InputLines Process(const InputLines& inputlines);
	InputLines Process(const InputLine& line);
	void FlushLinesFromTerminatedProcesses(InputLines& inputlines);

	double m_handleCacheTime;
	std::map<DWORD, std::string> m_lineBuffers;

	ProcessHandleCache m_handleCache;
	Timer m_timer;
};

} // namespace debugviewpp 
} // namespace fusion
