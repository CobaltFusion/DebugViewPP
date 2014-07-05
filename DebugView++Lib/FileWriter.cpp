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

namespace fusion {
namespace debugviewpp {

FileWriter::FileWriter(const std::wstring& filename, LogFile& logfile) :
	m_filename(filename),
	m_logfile(logfile)
{
	OpenDBLogFile(Str(filename).str());
	m_thread = boost::thread(&FileWriter::Process, this);
}

FileWriter::~FileWriter()
{
}

void FileWriter::OpenDBLogFile(std::string filename)
{
	bool newLogFile = !FileExists(filename.c_str());
	m_ofstream.open(filename, std::ofstream::app);
	if (newLogFile)
	{
		WriteLine(0.0, FILETIME(), 0, "DebugView++.exe", g_debugViewPPIdentification);
	}
}

std::ostream& operator<<(std::ostream& os, const FILETIME& ft)
{
	uint64_t hi = ft.dwHighDateTime;
	uint64_t lo = ft.dwLowDateTime;
	return os << ((hi << 32) | lo);
}

void FileWriter::Process()
{
	//todo: we need locking on Logfile, think of ClearLog() 
	// also, reading the .dblog file does not work correctly
	int writeIndex = 0;
	for(;;)
	{
		while (m_logfile.Count() > writeIndex)
		{
			auto msg = m_logfile[writeIndex];
			writeIndex++;
			WriteLine(msg.time, msg.systemTime, msg.processId, msg.processName, msg.text);
		}
		m_ofstream.flush();
		Sleep(1000);
	}
}

void FileWriter::WriteLine(double time, FILETIME filetime, DWORD pid, const std::string& processName, const std::string& message)
{
	m_ofstream <<
		time << '\t' <<
		filetime << '\t'<<
		pid << '\t'<<
		processName << '\t'<<
		message << '\t\r\n';
}

} // namespace debugviewpp 
} // namespace fusion
