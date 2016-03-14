// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <fstream>
#include "FileIO.h"
#include "Win32/Win32Lib.h"
#include "DebugView++Lib/LogSource.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

class FileReader : public LogSource
{
public:
	FileReader(Timer& timer, ILineBuffer& linebuffer, FileType::type filetype, const std::wstring& filename);
	virtual ~FileReader();

	virtual void Initialize();
	virtual bool AtEnd() const;
	virtual HANDLE GetHandle() const;
	virtual void Notify();
	virtual void PreProcess(Line& line) const;

protected:
	virtual void AddLine(const std::string& line);
	std::string m_filename;
	std::string m_name;
	FileType::type m_fileType;

private:
	void SafeAddLine(const std::string& line);
	void ReadUntilEof();

	bool m_end;
	Win32::ChangeNotificationHandle m_handle;
	std::ifstream m_ifstream;
	std::string m_filenameOnly;
	bool m_initialized;
};

} // namespace debugviewpp 
} // namespace fusion
