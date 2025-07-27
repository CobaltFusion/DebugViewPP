// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <cassert>
#include <locale>
#include <codecvt>
#include <filesystem>
#include <iostream>
#include "CobaltFusion/stringbuilder.h"
#include "CobaltFusion/Str.h"
#include "DebugViewppLib/FileIO.h"
#include "DebugViewppLib/BinaryFileReader.h"
#include "DebugViewppLib/LineBuffer.h"
#include "DebugViewppLib/Line.h"

namespace fusion {
namespace debugviewpp {

BinaryFileReader::BinaryFileReader(Timer& timer, ILineBuffer& linebuffer, FileType::type filetype, const std::wstring& filename) :
    LogSource(timer, SourceType::File, linebuffer),
    m_filename(filename),
    m_name(Str(std::filesystem::path(filename).filename().string()).str()),
    m_fileType(filetype),
    m_handle(FindFirstChangeNotification(std::filesystem::path(m_filename).parent_path().wstring().c_str(), 0, FILE_NOTIFY_CHANGE_SIZE)), //todo: maybe using FILE_NOTIFY_CHANGE_LAST_WRITE could have benefits, not sure what though.
    m_wifstream(m_filename, std::ios::binary),
    m_filenameOnly(std::filesystem::path(m_filename).filename().wstring()),
    m_initialized(false)
{
    switch (filetype)
    {
    case FileType::UTF16LE:
        m_wifstream.imbue(std::locale(m_wifstream.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::codecvt_mode(std::little_endian | std::consume_header)>));
        break;
    case FileType::UTF16BE:
        m_wifstream.imbue(std::locale(m_wifstream.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>));
        break;
    case FileType::UTF8:
        m_wifstream.imbue(std::locale(m_wifstream.getloc(), new std::codecvt_utf8<wchar_t>));
        break;
    default:
        assert(!"This BinaryFileReader filetype was not implemented!");
        break;
    }
    SetDescription(filename);
}

BinaryFileReader::~BinaryFileReader() = default;

void BinaryFileReader::Initialize()
{
    if (m_initialized)
    {
        return;
    }
    m_initialized = true;

    if (m_wifstream.is_open())
    {
        ReadUntilEof();
        Abort();
    }
}

HANDLE BinaryFileReader::GetHandle() const
{
    return m_handle.get();
}

void BinaryFileReader::Notify()
{
    ReadUntilEof();
    FindNextChangeNotification(m_handle.get());
}

void BinaryFileReader::ReadUntilEof()
{
    std::wstring line;
    int i = 0;
    while (std::getline(m_wifstream, line))
    {
        if ((++i % 1500) == 0)
        {
            m_update();
        }
        AddLine(Str(line));
    }
    m_update();

    if (m_wifstream.eof())
    {
        m_wifstream.clear(); // clear EOF condition

        // resync to end of file, even if the file shrunk
        auto lastReadPosition = m_wifstream.tellg();
        m_wifstream.seekg(0, std::wifstream::end);
        auto length = m_wifstream.tellg();
        if (length > lastReadPosition)
        {
            m_wifstream.seekg(lastReadPosition);
        }
        else if (length != lastReadPosition)
        {
            AddInternal(stringbuilder() << "file shrank, resynced at offset " << length);
        }
    }
    else
    {
        // Some error other then EOF occured
        AddInternal("Stopped tailing " + Str(m_filename).str());
        LogSource::Abort();
    }
}

void BinaryFileReader::AddLine(const std::string& line)
{
    AddInternal(line);
}

void BinaryFileReader::PreProcess(Line& line) const
{
    line.processName = Str(m_filenameOnly).str();
}

boost::signals2::connection BinaryFileReader::SubscribeToUpdate(UpdateSignal::slot_type slot)
{
    return m_update.connect(slot);
}

// todo: Reading support for more filetypes, maybe not, who logs in unicode anyway?
// see #107
// postpone until we have a valid usecase
// ANSI/ASCII
// UTF-8
// UTF-16
// UTF-8 NO BOM ?
// UTF-16 NO BOM ?
// UTF-16 Big Endian
// UTF-16 Big Endian NO BOM?
// Unicode ASCII escaped.

// http://stackoverflow.com/questions/10504044/correctly-reading-a-utf-16-text-file-into-a-string-without-external-libraries

// check a utf-16 file will always contain an even-amount of bytes?
// std::wifstream ifs(m_filename, std::ios::binary);
// fin.imbue(std::locale(fin.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>));
// for(wchar_t c; fin.get(c); ) std::cout << std::showbase << std::hex << c << '\n';

} // namespace debugviewpp
} // namespace fusion
