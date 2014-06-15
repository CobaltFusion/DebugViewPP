// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <boost/utility.hpp>
#include "Win32Lib/utilities.h"
#include "DebugView++Lib/LogSource.h"
#include <boost/thread.hpp>

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

class PassiveLogSource : public LogSource
{
public:
	PassiveLogSource(SourceType::type sourceType, ILineBuffer& linebuffer);
	virtual void Wakeup();
	virtual HANDLE GetHandle() const;
	virtual void Notify();
	virtual void AddLines() = 0;
private:
	Handle m_handle;
	boost::mutex m_mutex;
};

} // namespace debugviewpp 
} // namespace fusion
