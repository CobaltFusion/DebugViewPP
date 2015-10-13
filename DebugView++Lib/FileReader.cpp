// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <boost/filesystem.hpp>
#include "CobaltFusion/stringbuilder.h"
#include "DebugView++Lib/FileIO.h"
#include "DebugView++Lib/FileReader.h"
#include "DebugView++Lib/LineBuffer.h"
#include "DebugView++Lib/Line.h"

namespace fusion {
namespace debugviewpp {

FileReader::FileReader(Timer& timer, ILineBuffer& linebuffer, FileType::type filetype, const std::wstring& filename) :
	LogSource(timer, SourceType::File, linebuffer),
	m_end(true),
	m_filename(Str(filename).str()),
	m_fileType(filetype),
	m_name(Str(boost::filesystem::wpath(filename).filename().string()).str()),
	m_handle(FindFirstChangeNotification(boost::filesystem::wpath(m_filename).parent_path().wstring().c_str(), false, FILE_NOTIFY_CHANGE_SIZE)), //todo: maybe using FILE_NOTIFY_CHANGE_LAST_WRITE could have benefits, not sure what though.
	m_ifstream(m_filename, std::ios::in),
	m_filenameOnly(boost::filesystem::wpath(m_filename).filename().string()),
	m_initialized(false)
{
	SetDescription(filename);
}

FileReader::~FileReader()
{
}

void FileReader::Initialize()
{
	if (m_initialized)
	{
		return;
	}
	m_initialized = true;

	if (m_ifstream.is_open())
	{
		ReadUntilEof();
		m_end = false;
	}
}

bool FileReader::AtEnd() const
{
	return m_end;
}

HANDLE FileReader::GetHandle() const
{
	return m_handle.get();
}

void FileReader::Notify()
{
	ReadUntilEof();
	FindNextChangeNotification(m_handle.get());
}

void FileReader::ReadUntilEof()
{
	std::string line;
	while (std::getline(m_ifstream, line))
		AddLine(line);

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
			Add(stringbuilder() << "file shrank, resynced at offset " << length);
	}
	else
	{
		// Some error other then EOF occured
		Add("Stopped tailing " + m_filename);
		m_end = true;
	}
}


void FileReader::AddLine(const std::string& line)
{
	Add(line);
}

void FileReader::PreProcess(Line& line) const
{
	line.processName = m_filenameOnly;
}

} // namespace debugviewpp 
} // namespace fusion
