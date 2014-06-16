// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <boost/filesystem.hpp>
#include "DebugView++Lib/FileIO.h"
#include "DebugView++Lib/FileWriter.h"

namespace fusion {
namespace debugviewpp {

FileWriter::FileWriter(const std::wstring& filename) :
	m_filename(filename)
{
	m_thread = boost::thread(&FileWriter::Process, this);
}

FileWriter::~FileWriter()
{
}

void FileWriter::Process()
{

}

} // namespace debugviewpp 
} // namespace fusion
