// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <string>
#include <atlstr.h>
#include "Win32Lib/utilities.h"

namespace fusion {

void ScopedCursorDeleter::operator()(pointer p) const
{
	if (p != nullptr)
		SetCursor(p);
}

ScopedCursor::ScopedCursor(HCURSOR hCursor) :
	unique_ptr(GetCursor(hCursor))
{
}

void ScopedCursor::reset(HCURSOR hCursor)
{
	unique_ptr::reset(GetCursor(hCursor));
}

HCURSOR ScopedCursor::GetCursor(HCURSOR hCursor)
{
	return hCursor ? SetCursor(hCursor) : nullptr;
}

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

Timer::Timer() :
	m_offset(0),
	m_init(false),
	m_timerUnit(0.0)
{
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	if (li.QuadPart == 0)
		throw std::runtime_error("QueryPerformanceCounter not supported!");
	m_timerUnit = 1./li.QuadPart;
}

void Timer::Reset()
{
	m_offset = 0;
	m_init = false;
}

double Timer::Get()
{
	auto ticks = GetTicks();
	if (!m_init)
	{
		m_offset = ticks;
		m_init = true;
	}
	return (ticks - m_offset)*m_timerUnit;
}

long long Timer::GetTicks() const
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return li.QuadPart;
}

std::wstring GetDlgItemText(const CWindow& wnd, int idc)
{
	CString text;
	wnd.GetDlgItemText(idc, text);
	return std::wstring(text, text.GetLength());
}

} // namespace fusion
