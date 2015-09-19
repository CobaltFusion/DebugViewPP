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
#include "DebugView++Lib/FileReader.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

class DBLogReader : public FileReader
{
public:
	explicit DBLogReader(Timer& timer, ILineBuffer& linebuffer, FileType::type filetype, const std::wstring& filename);
	virtual void AddLine(const std::string& line);
	virtual void PreProcess(Line& line) const;
private:
	void GetRelativeTime(Line& line);
	long m_linenumber;
	FILETIME m_firstFiletime;
	USTimeConverter m_converter;
};

} // namespace debugviewpp 
} // namespace fusion
