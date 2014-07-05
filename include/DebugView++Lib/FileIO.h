// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <iosfwd>
#include "DebugView++Lib/Line.h"

namespace fusion {
namespace debugviewpp {

const std::string g_debugViewPPIdentification = "File Identification Header, DebugView++ Format Version 1";

struct FileType
{
	enum type
	{
		Unknown = 0,
		DebugViewPP,			// identified by first line (header) in file "0\t0\t0\tDebugView++\tFile Identification Header, DebugView++ v1.x.x.x"  (4 tabs)
		Sysinternals,			// identified by <line>\t<time>\t<message>\r\n (line containing 2 tabs + 1 microsoft newline)			    // kernel log message
								//  _or_         <line>\t<time>\t[pid] <message>\r\n (line containing 2 tabs + 1 microsoft newline)			// process log message
		AsciiText			    // and other file is treaded as if it was ASCII-text encoded, which is UTF8 compatible as long as nu actual unicode character are encoded.
	};
};

std::string FileTypeToString(FileType::type value);

bool FileExists(const char *filename);
FileType::type IdentifyFile(std::string filename);

std::istream& ReadLogFileMessage(std::istream& is, Line& line);

bool ReadSysInternalsLogFileMessage(const std::string& data, Line& line);
bool ReadLogFileMessage(const std::string& data, Line& line);

} // namespace debugviewpp 
} // namespace fusion
