//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <sstream>
#include <cmath>
#include <boost/date_time/local_time/local_time.hpp> 

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

namespace gj {

class ScopedCursor : boost::noncopyable
{
public:
	explicit ScopedCursor(HCURSOR cursor);
	~ScopedCursor();

private:
	HCURSOR m_cursor;
};

template <class CharType, class Traits = std::char_traits<CharType>, class Allocator = std::allocator<CharType>>
class basic_stringbuilder
{
public:
	typedef std::basic_string<CharType, Traits, Allocator> string_type;

	template <typename T>
	basic_stringbuilder& operator<<(const T& t)
	{
		m_ss << t;
		return *this;
	}

	string_type str() const
	{
		return m_ss.str();
	}

	const CharType* c_str() const
	{
		return m_ss.str().c_str();
	}

	operator string_type() const
	{
		return m_ss.str();
	}

private:
	std::basic_ostringstream<CharType, Traits, Allocator> m_ss;
};

typedef basic_stringbuilder<char> stringbuilder;
typedef basic_stringbuilder<wchar_t> wstringbuilder;

std::wstring MultiByteToWideChar(const std::string& str);
std::string WideCharToMultiByte(const std::wstring& str);

class Str
{
public:
	explicit Str(const std::string& s) :
		m_str(s)
	{
	}

	explicit Str(const char* s) :
		m_str(s)
	{
	}

	explicit Str(const std::wstring& s) :
		m_str(WideCharToMultiByte(s))
	{
	}

	explicit Str(const wchar_t* s) :
		m_str(WideCharToMultiByte(s))
	{
	}
	
	std::string str() const
	{
		return m_str;
	}

	const char* c_str() const
	{
		return m_str.c_str();
	}

	operator std::string() const
	{
		return m_str;
	}

	operator const char*() const
	{
		return m_str.c_str();
	}

private:
	std::string m_str;
};

class WStr
{
public:
	explicit WStr(const std::string& s) :
		m_str(MultiByteToWideChar(s))
	{
	}

	explicit WStr(const std::wstring& s) :
		m_str(s)
	{
	}

	std::wstring str() const
	{
		return m_str;
	}

	const wchar_t* c_str() const
	{
		return m_str.c_str();
	}

	operator std::wstring() const
	{
		return m_str;
	}

	operator const wchar_t*() const
	{
		return m_str.c_str();
	}

private:
	std::wstring m_str;
};

void ThrowWin32Error(DWORD error, const std::string& what);

void ThrowLastError(const std::string& what);

void ThrowLastError(const std::wstring& what);

std::wstring LoadString(int id);

std::wstring GetExceptionMessage();

class Timer
{
public:
	Timer();

	void Reset();
	double Get() const;

private:
	long long GetTicks() const;

	double m_timerUnit;
	long long m_offset;
};

template <typename T>
std::unique_ptr<T> make_unique()
{
	return std::unique_ptr<T>(new T);
}

template <typename T, typename A1>
std::unique_ptr<T> make_unique(A1&& a1)
{
	return std::unique_ptr<T>(new T(std::forward<A1>(a1)));
}

template <typename T, typename A1, typename A2>
std::unique_ptr<T> make_unique(A1&& a1, A2&& a2)
{
	return std::unique_ptr<T>(new T(std::forward<A1>(a1), std::forward<A2>(a2)));
}

template <typename T, typename A1, typename A2, typename A3>
std::unique_ptr<T> make_unique(A1&& a1, A2&& a2, A3&& a3)
{
	return std::unique_ptr<T>(new T(std::forward<A1>(a1), std::forward<A2>(a2), std::forward<A3>(a3)));
}

template <typename T>
T floor_to(double value)
{
	return static_cast<T>(std::floor(value));
}

std::wstring GetDlgItemText(const CWindow& wnd, int idc);

} // namespace gj
