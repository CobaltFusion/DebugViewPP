// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "LogSource.h"
#include "Win32Lib.h"

namespace fusion {
namespace debugviewpp {

class LogSources
{
public:
	LogSources();
	~LogSources();

	void Add(std::unique_ptr<LogSource> source);
	void Run();
	void Abort();

private:
	std::map<HANDLE, LogSource*> GetLogSourcesMap() const;
	std::vector<HANDLE> GetObjects() const;

	std::vector<std::unique_ptr<LogSource>> m_sources;
	Handle m_updateEvent;
	bool m_end;
	bool m_sourcesDirty;

	// make sure the thread is last to initialize
	boost::thread m_thread;
};

} // namespace debugviewpp 
} // namespace fusion
