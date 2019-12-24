// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <fstream>
#include "DebugView++Lib/Conversions.h"
#include "DebugView++Lib/LogSource.h"
#include <boost/signals2.hpp>

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

class BinaryFileReader : public LogSource
{
public:
    BinaryFileReader(Timer& timer, ILineBuffer& linebuffer, FileType::type filetype, const std::wstring& filename);
    ~BinaryFileReader() override;

    void Initialize() override;
    HANDLE GetHandle() const override;
    void Notify() override;
    void PreProcess(Line& line) const override;
    void AddLine(const std::string& line);

    using UpdateSignal = boost::signals2::signal<void()>;
    boost::signals2::connection SubscribeToUpdate(UpdateSignal::slot_type slot);

protected:
    std::wstring m_filename;
    std::string m_name;
    FileType::type m_fileType;

private:
    void ReadUntilEof();

    Win32::ChangeNotificationHandle m_handle;
    std::wifstream m_wifstream;
    std::wstring m_filenameOnly;
    bool m_initialized;
    UpdateSignal m_update;
};

} // namespace debugviewpp
} // namespace fusion
