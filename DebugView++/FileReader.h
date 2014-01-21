// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "Win32Lib.h"
#include "PipeReader.h"
#include "Process.h"
#include "DBWinBuffer.h"

namespace fusion {
namespace debugviewpp {

class FileReader : public LogSource
{
public:
	explicit FileReader(const std::wstring& filename);
    ~FileReader();

	virtual bool AtEnd() const;
	virtual HANDLE GetHandle() const;
	virtual Line GetLine();
	virtual Lines GetLines();

protected:
	virtual void Add(const std::string& line);
	Timer m_timer;

	boost::mutex m_linesMutex;
	Lines m_buffer;
	std::wstring m_filename;	
	std::string m_name;	

private:
	void Run();
    void Abort();

    bool m_end;
	ChangeNotificationHandle m_handle;
	boost::thread m_thread;
};

class DBLogReader : public FileReader
{
public:
	explicit DBLogReader(const std::wstring& filename);
	virtual HANDLE GetHandle() const;
	virtual Line GetLine();
private:
	virtual void Add(const std::string& line);
	FILETIME m_time;
};

bool ReadTime(const std::string& s, double& time);
bool ReadSystemTime(const std::string& text, const FILETIME& ftRef, FILETIME& ft);

} // namespace debugviewpp 
} // namespace fusion
