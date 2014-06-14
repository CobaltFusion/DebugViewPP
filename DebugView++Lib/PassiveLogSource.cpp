// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "Win32Lib/Win32Lib.h"
#include "DebugView++Lib/PassiveLogSource.h"
#include "DebugView++Lib/LineBuffer.h"

namespace fusion {
namespace debugviewpp {

PassiveLogSource::PassiveLogSource(SourceType::type sourceType, ILineBuffer& linebuffer) :
	LogSource(sourceType, linebuffer),
	m_handle(CreateEvent(NULL, TRUE, FALSE, L"WakeupEvent"))
{
}

HANDLE PassiveLogSource::GetHandle() const
{
	return m_handle.get();
}

void PassiveLogSource::Wakeup()
{
	boost::mutex::scoped_lock lock(m_mutex);
	SetEvent(m_handle.get());
}

void PassiveLogSource::Notify()
{
	AddLines();
	boost::mutex::scoped_lock lock(m_mutex);
	ResetEvent(m_handle.get());
}

} // namespace debugviewpp 
} // namespace fusion
