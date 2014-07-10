// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <boost/thread.hpp>
#include "PipeReader.h"
#include "Process.h"
#include "FileIO.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

class FileReader : public LogSource
{
public:
	explicit FileReader(Timer& timer, ILineBuffer& linebuffer, const std::wstring& filename);
    virtual ~FileReader();

	virtual void Initialize();
	virtual bool AtEnd() const;
	virtual HANDLE GetHandle() const;
	virtual void Notify();
	virtual void PreProcess(Line& line) const;
	virtual void AddLine(const std::string& line);

protected:
	std::string m_filename;	
	std::string m_name;	
	FileType::type m_fileType;

private:
	void ReadUntilEof();
    bool m_end;
	ChangeNotificationHandle m_handle;
	std::ifstream m_ifstream;
	std::string m_filenameOnly;
	bool m_initialized;
};

class DBLogReader : public FileReader
{
public:
	explicit DBLogReader(Timer& timer, ILineBuffer& linebuffer, const std::wstring& filename);
	virtual void AddLine(const std::string& line);
private:
	void GetRelativeTime(Line& line);
	bool m_firstline;
	FILETIME m_firstFiletime;
};

} // namespace debugviewpp 
} // namespace fusion
