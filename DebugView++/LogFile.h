//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>

namespace gj {

struct Message
{
	Message(const SYSTEMTIME& localTime, double time, DWORD processId, const std::wstring& processName, const std::string& text) :
		localTime(localTime), time(time), processId(processId), processName(processName), text(text)
	{
	}

	double time;
	SYSTEMTIME localTime;
	DWORD processId;
	std::wstring processName;
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
