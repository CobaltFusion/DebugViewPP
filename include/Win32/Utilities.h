// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <string>
#include <cmath>
#include <boost/date_time/local_time/local_time.hpp> 
#pragma warning(push, 3)
#include <boost/thread.hpp>
#pragma warning(pop)
#include <atlbase.h>
#include <atlwin.h>
#include "Win32/Win32Lib.h"

// Alternative to ATL standard BEGIN_MSG_MAP() with try block:
#define BEGIN_MSG_MAP_TRY(theClass) \
	BOOL theClass::ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID) \
	try \
	{ \
		BOOL bHandled = TRUE; \
		(hWnd); \
		(uMsg); \
		(wParam); \
		(lParam); \
		(lResult); \
		(bHandled); \
		switch(dwMsgMapID) \
		{ \
		case 0:

#define END_MSG_MAP_CATCH(handler) \
			break; \
		default: \
			ATLTRACE(ATL::atlTraceWindowing, 0, _T("Invalid message map ID (%i)\n"), dwMsgMapID); \
			ATLASSERT(FALSE); \
			break; \
		} \
		return FALSE; \
		} \
		catch (...) \
		{ \
			handler(); \
			return FALSE; \
		}

namespace fusion {

template <typename T, size_t N>
size_t array_size(T (&)[N])
{
	return N;
}

class ScopedCursor : boost::noncopyable
{
public:
	explicit ScopedCursor(HCURSOR hCursor);
	ScopedCursor(ScopedCursor&& sc);
	~ScopedCursor();

private:
	HCURSOR m_hCursor;
};

std::wstring LoadString(int id);

std::wstring GetExceptionMessage();

class Timer
{
public:
	Timer();
	Timer(LONGLONG quadPart);

	void Reset();
	double Get(long long ticks);
	double Get();

private:
	long long GetTicks() const;

	double m_timerUnit;
	bool m_init;
	long long m_offset;
	boost::mutex m_mutex;
};

template <typename T>
T floor_to(double value)
{
	return static_cast<T>(std::floor(value));
}

std::wstring GetDlgItemText(const CWindow& wnd, int idc);

FILETIME MakeFileTime(uint64_t t);

} // namespace fusion
