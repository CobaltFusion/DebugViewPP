// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <vector>
#include <iostream>
#include <shellapi.h>
#include "Win32Lib/Win32Lib.h"

#pragma comment(lib, "advapi32.lib")	// SetPrivilege

namespace fusion {

void LocalAllocDeleter::operator()(pointer p) const
{
	if (p != nullptr)
		LocalFree(p);
}

void GlobalAllocDeleter::operator()(pointer p) const
{
	if (p != nullptr)
		GlobalFree(p);
}

void HandleDeleter::operator()(pointer p) const
{
	if (p != nullptr && p != INVALID_HANDLE_VALUE)
		CloseHandle(p);
}

void ChangeNotificationHandleDeleter::operator()(pointer p) const
{
	if (p != nullptr && p != INVALID_HANDLE_VALUE)
		FindCloseChangeNotification(p);
}

GdiObjectSelection::GdiObjectSelection(HDC hdc, HGDIOBJ hObject) :
	m_hdc(hdc), m_hObject(SelectObject(hdc, hObject))
{
}

GdiObjectSelection::~GdiObjectSelection()
{
	SelectObject(m_hdc, m_hObject);
}

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

Win32Error::Win32Error(DWORD error, const std::string& what) :
	boost::system::system_error(error, boost::system::get_system_category(), what)
{
}

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

SYSTEMTIME GetSystemTime()
{
	SYSTEMTIME st;
	::GetSystemTime(&st);
	return st;
}

SYSTEMTIME GetLocalTime()
{
	SYSTEMTIME st;
	::GetLocalTime(&st);
	return st;
}

FILETIME GetSystemTimeAsFileTime()
{
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	return ft;
}

FILETIME FileTimeToLocalFileTime(const FILETIME& ft)
{
	FILETIME ftLocal;
	if (!::FileTimeToLocalFileTime(&ft, &ftLocal))
		ThrowLastError("FileTimeToLocalFileTime");
	return ftLocal;
}

SYSTEMTIME FileTimeToSystemTime(const FILETIME& ft)
{
	SYSTEMTIME st;
	if (!::FileTimeToSystemTime(&ft, &st))
		ThrowLastError("FileTimeToSystemTime");
	return st;
}

FILETIME SystemTimeToFileTime(const SYSTEMTIME& st)
{
	FILETIME ft;
	if (!::SystemTimeToFileTime(&st, &ft))
		ThrowLastError("SystemTimeToFileTime");
	return ft;
}

Handle CreateFileMapping(HANDLE hFile, const SECURITY_ATTRIBUTES* pAttributes, DWORD protect, DWORD maximumSizeHigh, DWORD maximumSizeLow, const wchar_t* pName)
{
	Handle hMap(::CreateFileMappingW(hFile, const_cast<SECURITY_ATTRIBUTES*>(pAttributes), protect, maximumSizeHigh, maximumSizeLow, pName));
	if (!hMap)
		ThrowLastError(std::wstring(L"CreateFileMapping(\"") + pName + L"\")");

	return hMap;
}

Handle OpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId)
{
	Handle hProcessHandle(::OpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId));
	if (!hProcessHandle)
		ThrowLastError(L"OpenProcess");

	return hProcessHandle;
}

Handle CreateEvent(const SECURITY_ATTRIBUTES* pEventAttributes, bool manualReset, bool initialState, const wchar_t* pName)
{
	Handle hEvent(::CreateEventW(const_cast<SECURITY_ATTRIBUTES*>(pEventAttributes), manualReset, initialState, pName));
	if (!hEvent)
		ThrowLastError(std::wstring(L"CreateEvent(\"") + pName + L"\")");

	return hEvent;
}

Handle CreateMutex(const SECURITY_ATTRIBUTES* pMutexAttributes, bool initialOwner, const wchar_t* pName)
{
	Handle hMutex(::CreateMutexW(const_cast<SECURITY_ATTRIBUTES*>(pMutexAttributes), initialOwner, pName));
	if (!hMutex)
		ThrowLastError(std::wstring(L"CreateMutex(\"") + pName + L"\")");

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

WaitResult::WaitResult(bool signaled, int index) : 
	signaled(signaled),
	index(index)
{
}

WaitResult WaitForAnyObject(const std::vector<HANDLE>& handles, DWORD milliSeconds)
{
	auto rc = ::WaitForMultipleObjects(handles.size(), handles.data(), FALSE, milliSeconds);
	if (rc == WAIT_TIMEOUT)
		return WaitResult(false);
	if (rc == WAIT_FAILED)
		ThrowLastError("WaitForMultipleObjects");
	if (rc >= WAIT_OBJECT_0 && rc < WAIT_OBJECT_0+handles.size())
		return WaitResult(true, rc);
	else 
		throw std::runtime_error("WaitForMultipleObjects");
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

std::wstring RegGetStringValue(HKEY hKey, const wchar_t* valueName)
{
	long length = 0;
	long rc = ::RegQueryValue(hKey, valueName, nullptr, &length);
	if (rc != ERROR_SUCCESS)
		ThrowWin32Error(rc, "RegQueryValue");

	std::vector<wchar_t> data(length);
	rc = ::RegQueryValue(hKey, valueName, data.data(), &length);
	if (rc != ERROR_SUCCESS)
		ThrowWin32Error(rc, "RegQueryValue");
	return data.data();
}

std::wstring RegGetStringValue(HKEY hKey, const wchar_t* valueName, const wchar_t* defaultValue)
{
	DWORD type;
	DWORD length = 0;
	long rc = ::RegQueryValueEx(hKey, valueName, nullptr, &type, nullptr, &length);
	if (rc != ERROR_SUCCESS || type != REG_SZ)
		return defaultValue;

	std::vector<wchar_t> data(length);
	rc = ::RegQueryValueEx(hKey, valueName, nullptr, &type, reinterpret_cast<BYTE*>(data.data()), &length);
	if (rc != ERROR_SUCCESS)
		return defaultValue;

	return data.data();
}

DWORD RegGetDWORDValue(HKEY hKey, const wchar_t* valueName)
{
	DWORD type;
	DWORD value;
	DWORD count = sizeof(value);
	long rc = RegQueryValueEx(hKey, valueName, nullptr, &type, reinterpret_cast<BYTE*>(&value), &count);
	if (rc != ERROR_SUCCESS)
		ThrowWin32Error(rc, "RegQueryValueEx");
	if (type != REG_DWORD)
		throw std::runtime_error("Invalid registry key");

	return value;
}

DWORD RegGetDWORDValue(HKEY hKey, const wchar_t* valueName, DWORD defaultValue)
{
	DWORD type;
	DWORD value;
	DWORD count = sizeof(value);
	long rc = RegQueryValueEx(hKey, valueName, nullptr, &type, reinterpret_cast<BYTE*>(&value), &count);
	if (rc == ERROR_SUCCESS && type == REG_DWORD)
		return value;
	return defaultValue;
}

void SetPrivilege(HANDLE hToken, const wchar_t* privilege, bool enablePrivilege)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;
	if (!LookupPrivilegeValue(nullptr, privilege, &luid))
		ThrowLastError("LookupPrivilegeValue");

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = enablePrivilege ? SE_PRIVILEGE_ENABLED : 0;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr))
		ThrowLastError("AdjustTokenPrivileges");
}

void SetPrivilege(const wchar_t* privilege, bool enablePrivilege)
{
	HANDLE handle; 
	if (!::OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &handle))
		ThrowLastError("OpenProcessToken");

	Handle hToken(handle);
	SetPrivilege(hToken.get(), privilege, enablePrivilege);
}

ULONG_PTR GetParentProcessId()
{
	ULONG_PTR pbi[6];
	ULONG ulSize = 0;
	long (WINAPI* NtQueryInformationProcess)(HANDLE ProcessHandle, ULONG ProcessInformationClass, void* ProcessInformation, ULONG ProcessInformationLength, ULONG* pReturnLength);
	*(FARPROC *)&NtQueryInformationProcess = GetProcAddress(LoadLibraryA("NTDLL.DLL"), "NtQueryInformationProcess");
	if (NtQueryInformationProcess && NtQueryInformationProcess(GetCurrentProcess(), 0, &pbi, sizeof(pbi), &ulSize) >= 0 && ulSize == sizeof(pbi))
		return pbi[5];
	return (ULONG_PTR)-1;
}

std::vector<std::wstring> GetCommandLineArguments()
{
	int argc;
	HLocal args(CommandLineToArgvW(GetCommandLineW(), &argc));
	auto argv = static_cast<const wchar_t**>(args.get());
	return std::vector<std::wstring>(argv, argv + argc);
}

} // namespace fusion
