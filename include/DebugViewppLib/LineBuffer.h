// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#pragma once

#include <string>
#include "LogSource.h"

namespace fusion {
namespace debugviewpp {

class LogSource;

class ILineBuffer
{
public:
    virtual ~ILineBuffer() = 0;

    virtual void Add(double time, FILETIME systemTime, HANDLE handle, const std::string& message, const LogSource* pLogSource) = 0;
    virtual void Add(double time, FILETIME systemTime, DWORD pid, const std::string& processName, const std::string& message, const LogSource* pLogSource) = 0;
    virtual Lines GetLines() = 0;
    virtual bool Empty() const = 0;
};

} // namespace debugviewpp
} // namespace fusion
