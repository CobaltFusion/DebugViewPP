// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "stdafx.h"
#include "DebugView++Lib/LogSource.h"

namespace fusion {
namespace debugviewpp {

LogSource::LogSource(SourceType::type sourceType) : m_sourceType(sourceType)
{

}

LogSource::~LogSource()
{
}

std::wstring LogSource::GetDescription() const
{
	return m_description;
}

void LogSource::SetDescription(const std::wstring& description)
{
	m_description = description;
}

SourceType::type LogSource::GetSourceType() const
{
	return m_sourceType;
}

} // namespace debugviewpp 
} // namespace fusion
