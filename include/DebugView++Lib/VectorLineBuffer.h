// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "CobaltFusion/CircularBuffer.h"
#include "LineBuffer.h"
#include "ProcessHandleCache.h"

namespace fusion {
namespace debugviewpp {

struct VecLine
{
	VecLine(double time = 0.0, FILETIME systemTime = FILETIME(), HANDLE handle = nullptr, const std::string& message = "");

	double time;
	FILETIME systemTime;
	HANDLE handle;
	std::string message;
};

class VectorLineBuffer : public ILineBuffer
{
public:
	explicit VectorLineBuffer(size_t size);

	void Add(double time, FILETIME systemTime, HANDLE handle, const char* message);
	Lines GetLines();
private:
	boost::mutex m_linesMutex;
	std::vector<VecLine> m_buffer;
	std::vector<VecLine> m_backingBuffer;
	ProcessHandleCache m_handleCache;

	Lines ProcessLines(std::vector<VecLine>& lines);
};



} // namespace debugviewpp 
} // namespace fusion
