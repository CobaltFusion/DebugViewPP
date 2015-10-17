// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "LogSource.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

class TestSource : public LogSource
{
public:
	TestSource(Timer& timer, ILineBuffer& linebuffer);

	virtual bool AtEnd() const;
	virtual HANDLE GetHandle() const;
	virtual void Notify();
};

} // namespace debugviewpp 
} // namespace fusion
