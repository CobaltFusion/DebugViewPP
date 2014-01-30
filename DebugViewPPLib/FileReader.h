// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "PipeReader.h"
#include "Process.h"
#include <boost/thread.hpp>

namespace fusion {
namespace debugviewpp {

class FileReader : public LogSource
{
public:
	explicit FileReader(const std::wstring& filename);
    ~FileReader();

	virtual bool AtEnd() const;
	virtual HANDLE GetHandle() const;
	virtual void Notify();
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
	virtual void Notify();

private:
	virtual void Add(const std::string& line);
	FILETIME m_time;
};

} // namespace debugviewpp 
} // namespace fusion
