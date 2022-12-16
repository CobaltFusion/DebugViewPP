// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#pragma once

#include <fstream>
#include "FileIO.h"
#include "Win32/Win32Lib.h"
#include "DebugviewppLib/LogSource.h"
#include <boost/signals2.hpp>

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

class FileReader : public LogSource
{
public:
    FileReader(Timer& timer, ILineBuffer& linebuffer, FileType::type filetype, const std::wstring& filename, bool keeptailing);
    ~FileReader() override;

    void Initialize() override;
    using UpdateSignal = boost::signals2::signal<void()>;
    boost::signals2::connection SubscribeToUpdate(UpdateSignal::slot_type slot);

    void Abort() override;
    HANDLE GetHandle() const override;
    void Notify() override;
    void PreProcess(Line& line) const override;

protected:
    virtual void AddLine(const std::string& line);
    std::string m_filename;
    std::string m_name;
    FileType::type m_fileType;

private:
    void SafeAddLine(const std::string& line);
    void ReadUntilEof();
    void PollThread();

    Win32::ChangeNotificationHandle m_handle;
    std::ifstream m_ifstream;
    std::string m_filenameOnly;
    bool m_initialized;
    std::string m_line;
    bool m_keeptailing;
    UpdateSignal m_update;
    std::thread m_thread;
};

} // namespace debugviewpp
} // namespace fusion
