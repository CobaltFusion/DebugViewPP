// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include <vector>
#include "DebugView++Lib/Colors.h"
#include "DebugView++Lib/ProcessInfo.h"
#include "IndexedStorageLib/IndexedStorage.h"

namespace fusion {
namespace debugviewpp {

struct Message
{
	Message(double time, FILETIME systemTime, DWORD pid, const std::string processName, const std::string& msg, COLORREF color = Colors::BackGround);

	double time;
	FILETIME systemTime;
	DWORD processId;
	std::string processName;
	std::string text;
	COLORREF color;
};

class LogFile
{
	struct InternalMessage
	{
		InternalMessage(double time, FILETIME systemTime, DWORD uid) :
			time(time), systemTime(systemTime), uid(uid)
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
	mutable indexedstorage::SnappyStorage m_storage;
//	indexedstorage::VectorStorage m_storage;
};

} // namespace debugviewpp 
} // namespace fusion
