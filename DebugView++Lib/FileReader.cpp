// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <cassert>
#include <filesystem>
#include "CobaltFusion/stringbuilder.h"
#include "Win32/Win32Lib.h"
#include "DebugView++Lib/FileIO.h"
#include "DebugView++Lib/FileReader.h"
#include "DebugView++Lib/LineBuffer.h"
#include "DebugView++Lib/Line.h"

namespace fusion {
namespace debugviewpp {

namespace fs = std::experimental::filesystem;

FileReader::FileReader(Timer& timer, ILineBuffer& linebuffer, FileType::type filetype, const std::wstring& filename, bool keeptailing) :
	LogSource(timer, SourceType::File, linebuffer),
	m_filename(Str(filename).str()),
	m_name(Str(fs::path(filename).filename().string()).str()),
	m_fileType(filetype),
	m_handle(FindFirstChangeNotification(fs::path(m_filename).parent_path().wstring().c_str(), false, FILE_NOTIFY_CHANGE_SIZE)), //todo: maybe adding FILE_NOTIFY_CHANGE_LAST_WRITE could have benefits, not sure what though.
	m_ifstream(m_filename, std::ios::in),
	m_filenameOnly(std::experimental::filesystem::path(m_filename).filename().string()),
	m_initialized(false),
	m_keeptailing(keeptailing),
	m_thread([this] { PollThread(); })
{
	SetDescription(filename);
}

FileReader::~FileReader()
{
}

void FileReader::Abort()
{
	LogSource::Abort();
	if (m_thread.joinable())
		m_thread.join();
}

void FileReader::PollThread()
{
	// FILE_NOTIFY_CHANGE_SIZE is broken on windows vista and above in that it does not
	// trigger this notification until a) a cache timeout of N? (unspecified on MSDN, but > 30 seconds) or b) someone queries the status of the file
	// both are not useful when tailing a file.
	// references:
	// https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-writefile
	// https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-findfirstchangenotificationa
	// MSDN: 'For operating systems that use extensive caching, detection occurs only when the cache is sufficiently flushed.'
	// affect: timestamps for lines read from logfiles have > 500ms accuracy and unittests are slow.

	uintmax_t filesize = fs::file_size(fs::path(m_filename));
	ReadUntilEof();
	while (!AtEnd())
	{
		if (Win32::WaitForSingleObject(m_handle.get(), 500))
		{
			// we got a FILE_NOTIFY_CHANGE_SIZE event
			ReadUntilEof();
			FindNextChangeNotification(m_handle.get());
		}
		else
		{
			// timeout occurred, poll the filesize
			uintmax_t newFilesize = fs::file_size(fs::path(m_filename));
			if (filesize != newFilesize)
			{
				filesize = newFilesize;
				ReadUntilEof();
			}
		}
	}
}

void FileReader::Initialize()
{
	//if (m_initialized)
	//	return;

	//m_initialized = true;
	//if (m_ifstream.is_open())
	//{
	//	ReadUntilEof();
	//}
}

boost::signals2::connection FileReader::SubscribeToUpdate(UpdateSignal::slot_type slot)
{
	return m_update.connect(slot);
}

HANDLE FileReader::GetHandle() const
{
	return INVALID_HANDLE_VALUE; // m_handle.get();
}

void FileReader::Notify()
{
	//assert(m_handle);
	//ReadUntilEof();
	//if (!m_keeptailing)
	//{
	//	Abort();
	//	return;
	//}
	//FindNextChangeNotification(m_handle.get());
}

void FileReader::ReadUntilEof()
{
	std::string line;
	int count = 0;
	while (std::getline(m_ifstream, line))
	{
		m_line += line;
		if ((++count % 5000) == 0)
			m_update();
		if (m_ifstream.eof())
		{
			// the line ended without a newline character
		}
		else
		{
			SafeAddLine(m_line);
			m_line.clear();
		}
	}
	m_update();

	if (m_ifstream.eof())
	{
		m_ifstream.clear(); // clear EOF condition

		// resync to end of file, even if the file shrunk
		auto lastReadPosition = m_ifstream.tellg();
		m_ifstream.seekg(0, m_ifstream.end);
		auto length = m_ifstream.tellg();
		if (length > lastReadPosition)
			m_ifstream.seekg(lastReadPosition);
		else if (length != lastReadPosition)
		{
			m_line.clear();
			Add(stringbuilder() << "file shrank, resynced at offset " << length);
		}
	}
	else
	{
		// Some error other then EOF occured
		Add("Stopped tailing " + m_filename);
		LogSource::Abort();
	}
	m_update();
}

void FileReader::SafeAddLine(const std::string& line)
{
	try
	{
		AddLine(line);
	}
	catch (std::exception& e)
	{
		Add(stringbuilder() << "Error parsing line: " << e.what());
	}
}

void FileReader::AddLine(const std::string& line)
{
	// AnyFileReader and BinaryFileReader override this method
	Add(line);
}

void FileReader::PreProcess(Line& line) const
{
	line.processName = m_filenameOnly;
}

} // namespace debugviewpp
} // namespace fusion
