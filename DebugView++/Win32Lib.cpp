//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://boosttestui.wordpress.com/ for the boosttestui home page.

#include "stdafx.h"
#include "Win32Lib.h"
#include <vector>

namespace gj {

std::wstring MultiByteToWideChar(const char* str, int len)
{
	int buf_size = len + 2;
	std::vector<wchar_t> buf(buf_size);
	int write_len = ::MultiByteToWideChar(0, 0, str, len, buf.data(), buf_size);
	return std::wstring(buf.data(), buf.data() + write_len);
}

std::wstring MultiByteToWideChar(const char* str)
{
	return MultiByteToWideChar(str, strlen(str));
}

std::wstring MultiByteToWideChar(const std::string& str)
{
	return MultiByteToWideChar(str.c_str(), str.size());
}

std::string WideCharToMultiByte(const wchar_t* str, int len)
{
	int buf_size = len*2 + 2;
	std::vector<char> buf(buf_size);
	int write_len = ::WideCharToMultiByte(0, 0, str, len, buf.data(), buf.size(), nullptr, nullptr);
	return std::string(buf.data(), buf.data() + write_len);
}

std::string WideCharToMultiByte(const wchar_t* str)
{
	return WideCharToMultiByte(str, wcslen(str));
}

std::string WideCharToMultiByte(const std::wstring& str)
{
	return WideCharToMultiByte(str.c_str(), str.size());
}

class Win32Error : public boost::system::system_error
{
public:
	Win32Error(DWORD error, const std::string& what) :
		boost::system::system_error(error, boost::system::get_system_category(), what)
	{
	}
};

void ThrowWin32Error(DWORD error, const std::string& what)
{
	throw Win32Error(error, what);
}

void ThrowLastError(const std::string& what)
{
	ThrowWin32Error(GetLastError(), what);
}

void ThrowLastError(const std::wstring& what)
{
	ThrowLastError(WideCharToMultiByte(what));
}

CHandle CreateFileMapping(HANDLE hFile, const SECURITY_ATTRIBUTES* pAttributes, DWORD protect, DWORD maximumSizeHigh, DWORD maximumSizeLow, const wchar_t* pName)
{
	CHandle hMap(::CreateFileMappingW(hFile, const_cast<SECURITY_ATTRIBUTES*>(pAttributes), protect, maximumSizeHigh, maximumSizeLow, pName));
	if (!hMap)
		ThrowLastError("CreateFileMapping");

	return hMap;
}

CHandle CreateEvent(const SECURITY_ATTRIBUTES* pEventAttributes, bool manualReset, bool initialState, const wchar_t* pName)
{
	CHandle hEvent(::CreateEventW(const_cast<SECURITY_ATTRIBUTES*>(pEventAttributes), manualReset, initialState, pName));
	if (!hEvent)
		ThrowLastError("CreateEvent");

	return hEvent;
}

CHandle CreateMutex(const SECURITY_ATTRIBUTES* pMutexAttributes, bool initialOwner, const wchar_t* pName)
{
	CHandle hMutex(::CreateMutexW(const_cast<SECURITY_ATTRIBUTES*>(pMutexAttributes), initialOwner, pName));
	if (!hMutex)
		ThrowLastError("CreateMutex");

	return hMutex;
}

void WaitForSingleObject(HANDLE hObject)
{
	auto rc = ::WaitForSingleObject(hObject, INFINITE);
	switch (rc)
	{
	case WAIT_OBJECT_0: return;
	case WAIT_FAILED: ThrowLastError("WaitForSingleObject");
	case WAIT_TIMEOUT: assert(!"Unexpected timeout");
	default: throw std::runtime_error("WaitForSingleObject");
	}
}

bool WaitForSingleObject(HANDLE hObject, DWORD milliSeconds)
{
	auto rc = ::WaitForSingleObject(hObject, milliSeconds);
	switch (rc)
	{
	case WAIT_TIMEOUT: return false;
	case WAIT_OBJECT_0: return true;
	case WAIT_FAILED: ThrowLastError("WaitForSingleObject");
	default: throw std::runtime_error("WaitForSingleObject");
	}
}

MutexLock::MutexLock(HANDLE hMutex) :
	m_hMutex(hMutex)
{
	if (hMutex)
		WaitForSingleObject(hMutex);
}

MutexLock::~MutexLock()
{
	if (m_hMutex)
		ReleaseMutex(m_hMutex);
}

void MutexLock::Release()
{
	if (!m_hMutex)
		return;

	if (!ReleaseMutex(m_hMutex))
		ThrowLastError("ReleaseMutex");
	m_hMutex = nullptr;
}


MappedViewOfFile::MappedViewOfFile(HANDLE hFileMappingObject, DWORD access, DWORD offsetHigh, DWORD offsetLow, size_t bytesToMap) :
	m_ptr(::MapViewOfFile(hFileMappingObject, access, offsetHigh, offsetLow, bytesToMap))
{
	if (m_ptr == nullptr)
		ThrowLastError("MapViewOfFile");
}

MappedViewOfFile::~MappedViewOfFile()
{
	UnmapViewOfFile(m_ptr);
}

void* MappedViewOfFile::Ptr()
{
	return m_ptr;
}

const void* MappedViewOfFile::Ptr() const
{
	return m_ptr;
}

ComInitialization::ComInitialization(CoInit init)
{
	HRESULT hr = CoInitializeEx(nullptr, init);
	if (FAILED(hr))
		throw Win32Error(hr, "CoInitializeEx");
}

ComInitialization::~ComInitialization()
{
	CoUninitialize();
}

SYSTEMTIME GetLocalTime()
{
	SYSTEMTIME localTime;
	GetLocalTime(&localTime);
	return localTime;
}

} // namespace gj
