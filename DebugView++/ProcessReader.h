// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "PipeReader.h"
#include "Process.h"

namespace fusion {
namespace debugviewpp {

class ProcessReader : public LogSource
{
public:
	ProcessReader(const std::wstring& pathName, const std::wstring& args);

	virtual bool AtEnd() const;
	virtual HANDLE GetHandle() const;
	virtual Line GetLine();
	virtual Lines GetLines();

private:
	Process m_process;
	PipeReader m_stdout;
	PipeReader m_stderr;
};

} // namespace debugviewpp 
} // namespace fusion
