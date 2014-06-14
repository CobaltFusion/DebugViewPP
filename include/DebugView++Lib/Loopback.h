// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <boost/utility.hpp>
#include <boost/thread.hpp>
#include <boost/signals2.hpp>

#include "LogSource.h"

namespace fusion {
namespace debugviewpp {

class ILineBuffer;

struct LoopLine
{
	LoopLine(DWORD pid, const std::string& processName, const std::string& message, LogSource* logsource);
	DWORD pid;
	std::string processName;
	std::string message;
	LogSource* logsource;
};

class Loopback : public LogSource
{
public:
	explicit Loopback(ILineBuffer& linebuffer);
	~Loopback();

	virtual bool GetAutoNewLine() const;
	virtual bool AtEnd() const;
	virtual HANDLE GetHandle() const;
	virtual void Notify();
	virtual void PreProcess(Line& line) const;

	void AddMessage(DWORD pid, const char* processName, const char* message);
	void Signal();
private:
	void CheckForLines();

	std::vector<LoopLine> m_lines;
	Handle m_handle;
	boost::mutex m_mutex;
};

} // namespace debugviewpp 
} // namespace fusion
