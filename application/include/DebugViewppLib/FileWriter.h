// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <fstream>
#include <thread>

namespace fusion {
namespace debugviewpp {

std::ostream& operator<<(std::ostream& os, const FILETIME& ft);

class LogFile;

class FileWriter
{
public:
    FileWriter(const std::wstring& filename, LogFile& logfile);

private:
    void Run();

    std::ofstream m_ofstream;
    LogFile& m_logfile;
    std::thread m_thread;
};

} // namespace debugviewpp
} // namespace fusion
