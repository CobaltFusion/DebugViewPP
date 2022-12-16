// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#pragma once

#include <string>
#include <vector>

#include "windows.h"

namespace fusion {
namespace debugviewpp {

class LogSource;

struct Line
{
    Line(double time, FILETIME systemTime, HANDLE handle, const std::string& message, const LogSource* pLogSource);
    explicit Line(double time = 0.0, FILETIME systemTime = FILETIME(), DWORD pid = 0, const std::string& processName = std::string(), const std::string& message = std::string(), const LogSource* pLogSource = nullptr);

    Line(const Line&) = default;
    Line& operator=(const Line&) = delete;

    Line(Line&&) = default;
    Line& operator=(Line&&) = default;

    double time;
    FILETIME systemTime;
    HANDLE handle;
    DWORD pid;
    std::string processName;
    std::string message;
    const LogSource* pLogSource;
};

using Lines = std::vector<Line>;

} // namespace debugviewpp
} // namespace fusion
