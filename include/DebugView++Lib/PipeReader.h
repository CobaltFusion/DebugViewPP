// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "PolledLogSource.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

class PipeReader : public PolledLogSource
{
public:
	PipeReader(Timer& timer, ILineBuffer& lineBuffer, HANDLE hPipe, DWORD pid, const std::string& processName, long pollFrequency);
	virtual ~PipeReader();

	bool AtEnd() const override;
	void Poll() override;
	void Poll(PolledLogSource& logSource);

private:
	HANDLE m_hPipe;
	DWORD m_pid;
	std::string m_process;
	std::string m_buffer;
};

} // namespace debugviewpp 
} // namespace fusion
