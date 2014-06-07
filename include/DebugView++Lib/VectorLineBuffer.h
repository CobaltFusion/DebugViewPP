// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "LineBuffer.h"
#include "ProcessHandleCache.h"

namespace fusion {
namespace debugviewpp {

class VectorLineBuffer : public ILineBuffer
{
public:
	explicit VectorLineBuffer(size_t size);

	virtual void Add(double time, FILETIME systemTime, HANDLE handle, const char* message, LogSource* logsource);
	virtual void Add(double time, FILETIME systemTime, DWORD pid, const char* processName, const char* message, LogSource* logsource);
	InputLines GetLines();
private:
	boost::mutex m_linesMutex;
	InputLines m_buffer;
	InputLines m_backingBuffer;
};



} // namespace debugviewpp 
} // namespace fusion
