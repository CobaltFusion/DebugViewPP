// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DebugView++Lib/TestSource.h"

namespace fusion {
namespace debugviewpp {

TestSource::TestSource(Timer& timer, ILineBuffer& linebuffer) :
	LogSource(timer, SourceType::System, linebuffer)
{
}

bool TestSource::AtEnd() const
{
	return false;
}

HANDLE TestSource::GetHandle() const 
{
	return nullptr;
}

void TestSource::Notify()
{
}

} // namespace debugviewpp 
} // namespace fusion
