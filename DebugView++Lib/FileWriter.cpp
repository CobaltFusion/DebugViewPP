// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DebugView++Lib/LogFile.h"
#include "DebugView++Lib/FileIO.h"
#include "DebugView++Lib/FileWriter.h"

namespace fusion {
namespace debugviewpp {

FileWriter::FileWriter(const std::wstring& filename, LogFile& logfile) :
	m_logfile(logfile)
{
	OpenLogFile(m_ofstream, filename, OpenMode::Append);
	m_thread = std::thread(&FileWriter::Run, this);
}

void FileWriter::Run()
{
	//todo: we need locking on Logfile, think of ClearLog() 
	size_t writeIndex = 0;
	for (;;)
	{
		while (writeIndex < m_logfile.Count())
		{
			auto msg = m_logfile[writeIndex];
			++writeIndex;
			WriteLogFileMessage(m_ofstream, msg.time, msg.systemTime, msg.processId, msg.processName, msg.text);
		}
		m_ofstream.flush();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

} // namespace debugviewpp 
} // namespace fusion
