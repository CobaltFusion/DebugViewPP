// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <istream>
#include <algorithm>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include "Win32Lib.h"
#include "LogFile.h"
#include "FileIO.h"

namespace fusion {
namespace debugviewpp {

class TabSplitter
{
public:
	explicit TabSplitter(const std::string& text) :
		m_it(text.begin()),
		m_end(text.end())
	{
	}

	std::string TabSplitter::GetNext()
	{
		auto it = std::find(m_it, m_end, '\t');
		std::string s(m_it, it);
		m_it = it == m_end ? it : it + 1;
		return s;
	}

	std::string TabSplitter::GetTail() const
	{
		return std::string(m_it, m_end);
	}

private:
	std::string::const_iterator m_it;
	std::string::const_iterator m_end;
};

FILETIME MakeFileTime(uint64_t t)
{
	uint32_t mask = ~0U;
	FILETIME ft;
	ft.dwHighDateTime = (t >> 32) & mask;
	ft.dwLowDateTime = t & mask;
	return ft;
}

bool ReadTime(const std::string& s, double& time)
{
	std::istringstream is(s);
	return is >> time && is.eof();
}

bool ReadSystemTime(const std::string& text, const FILETIME& ftRef, FILETIME& ft)
{
	std::istringstream is(text);
	WORD h, m, s;
	char c1, c2, p1, p2;
	if (!((is >> h >> c1 >> m >> c2 >> s) && c1 == ':' && c2 == ':'))
		return false;
	if (is >> p1 >> p2 && p1 == 'P' && p2 == 'M')
		h += 12;

	SYSTEMTIME st = FileTimeToSystemTime(ftRef);
	st.wHour = h;
	st.wMinute = m;
	st.wSecond = s;
	st.wMilliseconds = 0;
	ft = SystemTimeToFileTime(st);
	return true;
}

std::istream& ReadLogFileMessage(std::istream& is, Line& line)
{
	std::string data;
	if (!std::getline(is, data))
		return is;
	if (!ReadLogFileMessage(data, line))
		is.setstate(std::ios_base::failbit);
	return is;
}

bool ReadLogFileMessage(const std::string& data, Line& line)
{
	TabSplitter split(data);
	auto col1 = split.GetNext();
	auto col2 = split.GetNext();
	auto col3 = split.GetTail();
	if (!col3.empty() && col3[0] == '[')
	{
		std::istringstream is3(col3);
		char c1, c2, c3;
		if (is3 >> std::noskipws >> c1 >> line.pid >> c2 >> c3 && c1 == '[' && c2 == ']' && c3 == ' ' && std::getline(is3, line.message))
		{
			if (!ReadTime(col2, line.time))
				ReadSystemTime(col2, line.systemTime, line.systemTime);
		}
		else
		{
			line.time = boost::lexical_cast<double>(col1);
			line.systemTime = MakeFileTime(boost::lexical_cast<uint64_t>(col2));
			line.pid = boost::lexical_cast<DWORD>(split.GetNext());
			line.processName = split.GetNext();
			line.message = split.GetTail();
		}
	}

	return true;
}

} // namespace debugviewpp 
} // namespace fusion
