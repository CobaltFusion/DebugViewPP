// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DebugView++Lib/Loopback.h"
#include "DebugView++Lib/LineBuffer.h"

namespace fusion {
namespace debugviewpp {

Loopback::Loopback(Timer& timer, ILineBuffer& linebuffer) : PassiveLogSource(timer, SourceType::System, linebuffer, 0)
{
	SetDescription(L"Loopback");
}

Loopback::~Loopback()
{
}

bool Loopback::GetAutoNewLine() const
{
	return true;
}

void Loopback::PreProcess(Line& line) const
{
	if (line.message.empty())
	{
		line.message = "\n";
	}
}

} // namespace debugviewpp 
} // namespace fusion
