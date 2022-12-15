// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#pragma once

#include <iosfwd>
#include "DebugView++Lib/Line.h"

namespace fusion {
namespace debugviewpp {

class USTimeConverter;

const std::string g_debugViewPPIdentification1 = "File Identification Header, DebugView++ Format Version 1";
const std::string g_debugViewPPIdentification2 = "File Identification Header, DebugView++ Format Version 2"; // not yet used

struct FileType
{
    enum type
    {
        Unknown = 0,
        DebugViewPP1, // identified by first line (header) in file "0\t0\t0\tDebugView++\tFile Identification Header, DebugView++ v1.x.x.x"  (4 tabs)
        DebugViewPP2, // identified by first line (header) in file "0\t0\t0\tDebugView++\tFile Identification Header, DebugView++ v1.x.x.x"  (4 tabs), // currently not used
        Sysinternals, // identified by <line>\t<time>\t<message>\r\n (line containing 2 tabs + 1 microsoft newline)                // kernel log message
                      //  _or_         <line>\t<time>\t[pid] <message>\r\n (line containing 2 tabs + 1 microsoft newline)            // process log message
        UTF8,
        UTF16BE,
        UTF16LE,
        AsciiText // any other file is treaded as if it was ASCII-text encoded, which is UTF8 compatible as long as no actual UTF8 characters are encoded.
    };
};

std::string FileTypeToString(FileType::type value);

bool FileExists(const char* filename);
FileType::type IdentifyFile(const std::wstring& filename);
bool IsBinaryFileType(FileType::type);

std::istream& ReadLogFileMessage(std::istream& is, Line& line);

bool ReadSysInternalsLogFileMessage(const std::string& data, Line& line, USTimeConverter& converter);
bool ReadLogFileMessage(const std::string& data, Line& line);

std::ostream& operator<<(std::ostream& os, const FILETIME& ft);

struct OpenMode
{
    enum type
    {
        Append,
        Truncate
    };
};

void OpenLogFile(std::ofstream& ofstream, const std::wstring& filename, OpenMode::type mode = OpenMode::Truncate);
void WriteLogFileMessage(std::ofstream& ofstream, double time, FILETIME filetime, DWORD pid, const std::string& processName, std::string message);

} // namespace debugviewpp
} // namespace fusion
