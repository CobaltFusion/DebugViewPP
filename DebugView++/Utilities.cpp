//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

#include "stdafx.h"
#include <string>
#include <atlstr.h>
#include "Utilities.h"

namespace gj {

std::wstring LoadString(int id)
{
	CString cs;
	if (!cs.LoadString(id))
		ThrowLastError("LoadString");
	return static_cast<const wchar_t*>(cs);
}

std::wstring GetExceptionMessage()
{
	try
	{
		throw;
	}
	catch (std::exception& e)
	{
		return MultiByteToWideChar(e.what());
	}
	catch (...)
	{
		return L"Unknown exception";
	}
}

Timer::Timer()
{
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	m_timerUnit = 1./li.QuadPart;	//todo: QuadPart can be zero!
	Reset();
}

void Timer::Reset()
{
	m_offset = GetTicks();
}

double Timer::Get() const
{
	return (GetTicks() - m_offset)*m_timerUnit;
}

long long Timer::GetTicks() const
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return li.QuadPart;
}

AccurateTime::AccurateTime() 
{
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	m_usfactor = li.QuadPart / 1000000.0;
	SyncToLocalTime();
}

boost::posix_time::ptime AccurateTime::Zero()
{
	return boost::posix_time::ptime(boost::posix_time::second_clock::date_type(1900, 1, 1));
}

// sync AccurateTime::GetTicks values to real time clock (including timezone if applicable)
void AccurateTime::SyncToLocalTime()
{
	auto now = boost::posix_time::second_clock::local_time();
	auto ptimeOffset = now - Zero();
	m_localtimeoffset = ptimeOffset.total_microseconds();
	m_offset = PerformanceCounterTicks();
}

long long AccurateTime::PerformanceCounterTicks() const
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return li.QuadPart;
}

// returns an absolute time as the number of microseconds that have elapsed since 12:00:00 midnight, January 1, 1900
// There are 1000 ticks in a millisecond.
TickType AccurateTime::GetTicks() const
{
	auto relativeTicks = static_cast<long long>((PerformanceCounterTicks() - m_offset) / m_usfactor);
	return m_localtimeoffset + relativeTicks;
}

std::string AccurateTime::GetLocalTimeString(TickType ticks)
{
	using namespace boost::posix_time;
	boost::posix_time::ptime time(Zero() + microseconds(ticks));

	std::stringstream ss;
	ss.imbue(std::locale(ss.getloc(), new boost::posix_time::time_facet("%H:%M:%S.%f")));
	ss << time;
	return ss.str();
}

TickType AccurateTime::GetRTCTime()
{
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);
	using namespace boost::posix_time;
	auto now = ptime(second_clock::date_type(localTime.wYear, localTime.wMonth, localTime.wDay), hours(localTime.wHour) + minutes(localTime.wMinute) + seconds(localTime.wSecond) + milliseconds(localTime.wMilliseconds));
	return (now - Zero()).total_microseconds();
}

std::wstring GetDlgItemText(const CWindow& wnd, int idc)
{
	CString text;
	wnd.GetDlgItemText(idc, text);
	return std::wstring(text, text.GetLength());
}

} // namespace gj
