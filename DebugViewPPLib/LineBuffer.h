// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "cobaltfusion.h"
#include "DBWinBuffer.h"
#include "win32Lib.h"
#include "Utilities.h"

namespace fusion {
namespace debugviewpp {

class LineBuffer : public CircularBuffer
{
public:
	explicit LineBuffer(size_t size);

	void Add(double time, FILETIME systemTime, HANDLE handle, const char* message);
	Lines GetLines();

protected:
	virtual bool Full() const;
};

} // namespace debugviewpp 
} // namespace fusion
