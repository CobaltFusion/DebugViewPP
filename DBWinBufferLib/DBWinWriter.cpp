// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "DBWinBufferLib/DBWinBuffer.h"
#include "DBWinBufferLib/DBWinWriter.h"

namespace fusion {
namespace debugviewpp {

DBWinWriter::DBWinWriter() :
	m_hBuffer(OpenFileMapping(FILE_MAP_WRITE, false, L"DBWIN_BUFFER")),
	m_dbWinBufferReady(OpenEvent(SYNCHRONIZE, false, L"DBWIN_BUFFER_READY")),
	m_dbWinDataReady(OpenEvent(EVENT_MODIFY_STATE, false, L"DBWIN_DATA_READY")),
	m_dbWinView(m_hBuffer.get(), FILE_MAP_WRITE, 0, 0, sizeof(DbWinBuffer))
{
}

void DBWinWriter::Write(DWORD pid, const std::string& message)
{
	if (!WaitForSingleObject(m_dbWinBufferReady.get(), 10000))
		return;

	auto pData = static_cast<DbWinBuffer*>(m_dbWinView.Ptr());
	pData->processId = pid;
	int length = std::min<int>(message.size(), sizeof(pData->data) - 1);
	std::copy(message.data(), message.data() + length, pData->data);
	pData->data[length] = '\0';

	SetEvent(m_dbWinDataReady.get());
}

} // namespace debugviewpp 
} // namespace fusion
