// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "PipeReader.h"
#include "Process.h"
#include "DBWinBuffer.h"

namespace fusion {
namespace debugviewpp {

class FileReader : public LogSource
{
public:
	FileReader(const std::wstring& filename);

	virtual bool AtEnd() const;
	virtual Lines GetLines();

private:
	void Run();
	void Add(const std::string& line);

	Lines m_buffer;
	Timer m_timer;
	mutable boost::mutex m_linesMutex;

	std::wstring m_filename;	
	Handle m_handle;
	boost::thread m_thread;
};

} // namespace debugviewpp 
} // namespace fusion
