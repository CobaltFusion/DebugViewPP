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
		time(msg.time), systemTime(msg.systemTime), processId(msg.processId), text(msg.text)
	{
	}

	Message(double time, FILETIME systemTime, DWORD pid, const std::string& msg) :
		time(time), systemTime(systemTime), processId(pid), text(msg)
	{
	}

	double time;
	FILETIME systemTime;
	DWORD processId;
	std::string text;
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
	void Add(const Message& msg);
	int Count() const;
	Message operator[](int i) const;

private:
	std::vector<Message> m_messages;
	ProcessInfo m_processInfo;
};

} // namespace fusion
