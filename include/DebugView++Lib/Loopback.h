// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "DebugView++Lib/LogSource.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

class Loopback : public LogSource
{
public:
	Loopback(Timer& timer, ILineBuffer& lineBuffer);
	virtual ~Loopback();

	virtual HANDLE GetHandle() const { throw std::exception("should never be called"); }
	virtual void Notify() {}

	virtual bool GetAutoNewLine() const;
	virtual void PreProcess(Line& line) const;
};

} // namespace debugviewpp 
} // namespace fusion
