// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <boost/filesystem.hpp>
#include "DebugView++Lib/FileIO.h"
#include "DebugView++Lib/FileWriter.h"
#include "CobaltFusion/dbgstream.h"
#include "Win32Lib/utilities.h"
#include <boost/algorithm/string.hpp>

namespace fusion {
namespace debugviewpp {

FileWriter::FileWriter(const std::wstring& filename, LogFile& logfile) :
	m_filename(filename),
	m_logfile(logfile)
{
	OpenLogFile(m_ofstream, Str(filename).str());
	m_thread = boost::thread(&FileWriter::Process, this);
}

FileWriter::~FileWriter()
{
}

void FileWriter::Process()
{
	//todo: we need locking on Logfile, think of ClearLog() 
	// also, reading the .dblog file does not work correctly
	size_t writeIndex = 0;
	for(;;)
	{
		while (m_logfile.Count() > writeIndex)
		{
			auto msg = m_logfile[writeIndex];
			writeIndex++;
			WriteLogFileMessage(m_ofstream, msg.time, msg.systemTime, msg.processId, msg.processName, msg.text);
		}
		m_ofstream.flush();
		Sleep(1000);
	}
}


} // namespace debugviewpp 
} // namespace fusion
