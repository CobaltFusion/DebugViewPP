//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <vector>

namespace gj {

struct Message
{
	Message(long long qpctime_, long long rtctime_, DWORD pid, const std::string& msg) :
		qpctime(qpctime_), rtctime(rtctime_), processId(pid), text(msg)
	{
	}

	long long qpctime;
	long long rtctime;
	DWORD processId;
	std::string text;
};

class LogFile
{
public:
	void Clear();
	void Add(const Message& msg);
	int Count() const;
	Message operator[](int i) const;

private:
	std::vector<Message> m_messages;
};

} // namespace gj
