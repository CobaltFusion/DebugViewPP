// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "PipeReader.h"
#include "PassiveLogSource.h"
#include "Process.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

class SocketReader : public PassiveLogSource
{
public:
	SocketReader(Timer& timer, ILineBuffer& linebuffer, const std::wstring& pathName, const std::wstring& args);
	virtual ~SocketReader();

	virtual bool AtEnd() const;
private:
	virtual void Poll();

};

} // namespace debugviewpp 
} // namespace fusion
