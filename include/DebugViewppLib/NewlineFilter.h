// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#pragma once

#include <string>
#include <unordered_map>

namespace fusion {
namespace debugviewpp {

struct Line;

class NewlineFilter
{
public:
    Lines Process(const Line& line);
    Lines FlushLinesFromTerminatedProcess(DWORD pid, HANDLE handle);

private:
    std::unordered_map<DWORD, std::string> m_lineBuffers;
};

} // namespace debugviewpp
} // namespace fusion
