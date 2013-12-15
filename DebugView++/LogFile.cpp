// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <vector>
#include "LogFile.h"

namespace fusion {

bool LogFile::Empty() const
{
	return m_messages.empty();
}

void LogFile::Clear()
{
	m_messages.clear();
}

void LogFile::Add(const Message& msg)
{
	m_messages.push_back(msg);
}
	
int LogFile::Count() const
{
	return m_messages.size();
}

Message LogFile::operator[](int i) const
{
	return m_messages[i];
}

} // namespace fusion
