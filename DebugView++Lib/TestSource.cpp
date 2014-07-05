// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DebugView++Lib/TestSource.h"
#include "DebugView++Lib/LineBuffer.h"

namespace fusion {
namespace debugviewpp {

TestSource::TestSource(Timer& timer, ILineBuffer& linebuffer) :
	LogSource(timer, SourceType::System, linebuffer)
{
}

TestSource::~TestSource()
{
}

bool TestSource::AtEnd() const
{
	return false;
}

HANDLE TestSource::GetHandle() const 
{
	return 0;
}

void TestSource::Notify()
{
}


} // namespace debugviewpp 
} // namespace fusion
