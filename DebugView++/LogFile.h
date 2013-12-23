// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include <vector>
#include "ProcessInfo.h"

namespace fusion {

struct Message
{
	Message(const Message& msg) : 
		time(msg.time), systemTime(msg.systemTime), processId(msg.processId), processName(msg.processName), 
			text(msg.text), handleValid(false), handle(0)
	{
	}

	Message(double time, FILETIME systemTime, DWORD pid, HANDLE handle_, const std::string& msg) :
		time(time), systemTime(systemTime), processId(pid), handle(handle_), text(msg), handleValid(true)
	{
	}

	Message(double time, FILETIME systemTime, DWORD pid, const std::string processName_, const std::string& msg) :
		time(time), systemTime(systemTime), processId(pid), processName(processName_), text(msg), handleValid(false)
	{
	}

	double time;
	FILETIME systemTime;
	DWORD processId;
	std::string processName;
	std::string text;
	HANDLE handle;
	bool handleValid;
};

class LogFile
{
	struct InternalMessage
	{
		InternalMessage(const Message& msg) : 
			time(msg.time), systemTime(msg.systemTime), uid(msg.processId), text(msg.text)
		{
		}

		InternalMessage(double time, FILETIME systemTime, DWORD uid_, const std::string& msg) :
			time(time), systemTime(systemTime), uid(uid_), text(msg)
		{
		}

		double time;
		FILETIME systemTime;
		DWORD uid;
		std::string text;
	};

public:
	bool Empty() const;
	void Clear();
	Message Add(const Message& msg);
	int Count() const;
	Message operator[](int i) const;

private:
	std::vector<InternalMessage> m_messages;
	ProcessInfo m_processInfo;
};

} // namespace fusion
