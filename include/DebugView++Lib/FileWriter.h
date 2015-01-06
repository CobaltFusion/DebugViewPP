// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <boost/thread.hpp>
#include "LogFile.h"

namespace fusion {
namespace debugviewpp {

std::ostream& operator<<(std::ostream& os, const FILETIME& ft);

class FileWriter
{
public:
	explicit FileWriter(const std::wstring& filename, LogFile& logfile);
    ~FileWriter();

private:
	void Process();
	
	std::wstring m_filename;
	std::ofstream m_ofstream;
	LogFile& m_logfile;
	boost::thread m_thread;
};

} // namespace debugviewpp 
} // namespace fusion
