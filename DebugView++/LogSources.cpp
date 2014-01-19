// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "stdafx.h"
#include "LogSources.h"

namespace fusion {
namespace debugviewpp {

LogSources::LogSources() : 
	m_end(false),
	m_sourcesDirty(false),
	m_updateEvent(CreateEvent(NULL, FALSE, FALSE, NULL)),
	m_thread(&LogSources::Run, this)
{

}

void LogSources::Add(std::unique_ptr<LogSource> source)
{
	m_sources.push_back(std::move(source));
	m_sourcesDirty = true;
	SetEvent(m_updateEvent.get());
}

void LogSources::Abort()
{
	m_end = true;
	SetEvent(m_updateEvent.get());
	m_thread.join();
}

std::map<HANDLE, LogSource*> LogSources::GetLogSourcesMap() const
{
	return std::map<HANDLE, LogSource*>();
}

std::vector<HANDLE> LogSources::GetObjects() const
{
	// collect handles from LogSources
	return std::vector<HANDLE>();
}

void LogSources::Run()
{
	for (;;)
	{
		auto objects = GetObjects();
		WaitForMultipleObjects(objects.size(), &objects[0], FALSE, 1000);
		if (m_end)
			break;
	}

}

LogSources::~LogSources()
{
}

} // namespace debugviewpp 
} // namespace fusion
