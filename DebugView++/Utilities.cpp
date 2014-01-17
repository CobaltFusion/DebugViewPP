// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <string>
#include <atlstr.h>
#include "Utilities.h"

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
	m_offset(0)
{
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	m_timerUnit = 1./li.QuadPart;	//todo: QuadPart can be zero!
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

std::wstring GetDlgItemText(const CWindow& wnd, int idc)
{
	CString text;
	wnd.GetDlgItemText(idc, text);
	return std::wstring(text, text.GetLength());
}

TabSplitter::TabSplitter(const std::string& text) :
	m_it(text.begin()),
	m_end(text.end())
{
};

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

FILETIME MakeFileTime(uint64_t t)
{
	uint32_t mask = ~0U;
	FILETIME ft;
	ft.dwHighDateTime = (t >> 32) & mask;
	ft.dwLowDateTime = t & mask;
	return ft;
}

} // namespace fusion
