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
#include "DebugView++Lib/Conversions.h"
#include "DebugView++Lib/LogSource.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

class BinaryFileReader : public LogSource
{
public:
	BinaryFileReader(Timer& timer, ILineBuffer& linebuffer, FileType::type filetype, const std::wstring& filename);
	virtual ~BinaryFileReader();

	virtual void Initialize();
	virtual bool AtEnd() const;
	virtual HANDLE GetHandle() const;
	virtual void Notify();
	virtual void PreProcess(Line& line) const;
	virtual void AddLine(const std::string& line);

protected:
	std::wstring m_filename;
	std::string m_name;
	FileType::type m_fileType;

private:
	void ReadUntilEof();

	bool m_end;
	Win32::ChangeNotificationHandle m_handle;
	std::wifstream m_wifstream;
	std::wstring m_filenameOnly;
	bool m_initialized;
};

} // namespace debugviewpp 
} // namespace fusion
