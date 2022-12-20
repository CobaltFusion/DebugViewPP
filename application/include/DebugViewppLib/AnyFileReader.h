// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "DebugviewppLib/Conversions.h"
#include "DebugviewppLib/FileReader.h"
#include <filesystem>

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

class AnyFileReader : public FileReader
{
public:
    AnyFileReader(Timer& timer, ILineBuffer& lineBuffer, FileType::type fileType, const std::wstring& filename, bool keeptailing);

    void AddLine(const std::string& line) override;
    void PreProcess(Line& line) const override;

private:
    void GetRelativeTime(Line& line);
    long m_linenumber;
    FILETIME m_firstFiletime;
    USTimeConverter m_converter;
    std::wstring m_filenameOnly;
};

} // namespace debugviewpp
} // namespace fusion
