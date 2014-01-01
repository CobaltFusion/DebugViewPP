// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include <vector>
#include "ProcessInfo.h"
#include "IndexedStorage.h"

namespace fusion {
namespace debugviewpp {

struct Message
{
	Message(double time, FILETIME systemTime, DWORD pid, const std::string processName_, const std::string& msg) :
		time(time), systemTime(systemTime), processId(pid), processName(processName_), text(msg)
	{
	}

	double time;
	FILETIME systemTime;
	DWORD processId;
	std::string processName;
	std::string text;
};

class LogFile
{
	struct InternalMessage
	{
		InternalMessage(double time, FILETIME systemTime, DWORD uid_) :
			time(time), systemTime(systemTime), uid(uid_)
		{
		}

		double time;
		FILETIME systemTime;
		DWORD uid;
	};

public:
	bool Empty() const;
	void Clear();
	void Add(const Message& msg);
	int Count() const;
	Message operator[](int i) const;

private:
	std::vector<InternalMessage> m_messages;
	ProcessInfo m_processInfo;
	mutable SnappyStorage m_storage;
//	VectorStorage m_storage;
};

} // namespace debugviewpp 
} // namespace fusion
