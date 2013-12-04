//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://boosttestui.wordpress.com/ for the boosttestui home page.

#pragma once

#include <string>
#include <boost/utility.hpp>
#include <boost/system/system_error.hpp>
#include <windows.h>

namespace gj {

std::wstring MultiByteToWideChar(const char* str, int len);
std::wstring MultiByteToWideChar(const char* str);
std::wstring MultiByteToWideChar(const std::string& str);

std::string WideCharToMultiByte(const wchar_t* str, int len);
std::string WideCharToMultiByte(const wchar_t* str);
std::string WideCharToMultiByte(const std::wstring& str);

void ThrowWin32Error(DWORD error, const std::string& what);
void ThrowLastError(const std::string& what);
void ThrowLastError(const std::wstring& what);

CHandle CreateFileMapping(HANDLE hFile, const SECURITY_ATTRIBUTES* pAttributes, DWORD protect, DWORD maximumSizeHigh, DWORD maximumSizeLow, const wchar_t* pName);
CHandle CreateEvent(const SECURITY_ATTRIBUTES* pEventAttributes, bool manualReset, bool initialState, const wchar_t* pName);
CHandle CreateMutex(const SECURITY_ATTRIBUTES* pMutexAttributes, bool initialOwner, const wchar_t* pName);

void WaitForSingleObject(HANDLE hObject);
bool WaitForSingleObject(HANDLE hObject, DWORD milliSeconds);

class MutexLock : boost::noncopyable
{
public:
	explicit MutexLock(HANDLE hMutex);
	~MutexLock();

	void Release();

private:
	HANDLE m_hMutex;
};

class MappedViewOfFile : boost::noncopyable
{
public:
	MappedViewOfFile(HANDLE hFileMappingObject, DWORD access, DWORD offsetHigh, DWORD offsetLow, size_t bytesToMap);
	~MappedViewOfFile();

	void* Ptr();
	const void* Ptr() const;

private:
	void* m_ptr;
};

class ComInitialization : boost::noncopyable
{
public:
	enum CoInit
	{
		ApartmentThreaded,
		Multithreaded
	};

	explicit ComInitialization(CoInit init);
	~ComInitialization();
};

} // namespace gj
