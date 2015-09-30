// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "CobaltFusion/CircularBuffer.h"
#include "Line.h"

namespace fusion {
namespace debugviewpp {

class LogSource;

class ILineBuffer
{
public:
	virtual void Add(double time, FILETIME systemTime, HANDLE handle, const char* message, const std::shared_ptr<LogSource>& pSource) = 0;
	virtual void Add(double time, FILETIME systemTime, DWORD pid, const char* processName, const char* message, const std::shared_ptr<LogSource>& pSource) = 0;
	virtual Lines GetLines() = 0;
	virtual bool Empty() const = 0;
};

} // namespace debugviewpp 
} // namespace fusion
