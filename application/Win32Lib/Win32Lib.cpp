// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "Win32/Win32Lib.h"

#include <vector>
#include <cassert>
#include <iostream>
#include <shellapi.h>
#include <io.h>
#include <fcntl.h>
#include "comdef.h"
#include <clocale>
#include <cstdlib>

#ifdef DACL_MODIFY
#include <AccCtrl.h>
#endif

#pragma comment(lib, "advapi32.lib") // SetPrivilege

namespace fusion {
namespace Win32 {

void LocalAllocDeleter::operator()(pointer p) const
{
    if (p != nullptr)
    {
        LocalFree(p);
    }
}

void GlobalAllocDeleter::operator()(pointer p) const
{
    if (p != nullptr)
    {
        GlobalFree(p);
    }
}

void HandleDeleter::operator()(pointer p) const
{
    if (p != nullptr && p != INVALID_HANDLE_VALUE)
    {
        CloseHandle(p);
    }
}

void ChangeNotificationHandleDeleter::operator()(pointer p) const
{
    if (p != nullptr && p != INVALID_HANDLE_VALUE)
    {
        FindCloseChangeNotification(p);
    }
}

GdiObjectSelection::GdiObjectSelection(HDC hdc, HGDIOBJ hObject) :
    m_hdc(hdc),
    m_hObject(SelectObject(hdc, hObject))
{
    if (m_hObject == nullptr)
    {
        ThrowLastError("SelectObject");
    }
}

GdiObjectSelection::~GdiObjectSelection()
{
    SelectObject(m_hdc, m_hObject);
}

ScopedTextColor::ScopedTextColor(HDC hdc, COLORREF color) :
    m_hdc(hdc),
    m_color(::SetTextColor(hdc, color))
{
    if (m_color == CLR_INVALID)
    {
        ThrowLastError("SetTextColor");
    }
}

ScopedTextColor::~ScopedTextColor()
{
    ::SetTextColor(m_hdc, m_color);
}

ScopedBkColor::ScopedBkColor(HDC hdc, COLORREF color) :
    m_hdc(hdc),
    m_color(::SetBkColor(hdc, color))
{
    if (m_color == CLR_INVALID)
    {
        ThrowLastError("SetBkColor");
    }
}

ScopedBkColor::~ScopedBkColor()
{
    ::SetBkColor(m_hdc, m_color);
}

ScopedTextAlign::ScopedTextAlign(HDC hdc, UINT align) :
    m_hdc(hdc),
    m_align(::SetTextAlign(hdc, align))
{
    if (m_align == GDI_ERROR)
    {
        ThrowLastError("SetTextAlign");
    }
}

ScopedTextAlign::~ScopedTextAlign()
{
    ::SetTextAlign(m_hdc, m_align);
}

std::wstring MultiByteToWideChar(std::string_view str)
{
    int buf_size = static_cast<int>(str.size() + 2);
    std::vector<wchar_t> buf(buf_size);
    size_t write_len = ::MultiByteToWideChar(0, 0, str.data(), static_cast<int>(str.size()), buf.data(), buf_size);
    return std::wstring(buf.data(), buf.data() + write_len);
}

std::wstring MultiByteToWideChar_std(std::string_view str) // supposedly more reliable, but not working.
{
    std::setlocale(LC_ALL, "");
    std::wstring ws(2 * str.size(), L'\0');
    ws.resize(std::mbstowcs(&ws[0], str.data(), str.size())); // Shrink to fit.
    return ws;
}

std::string WideCharToMultiByte(std::wstring_view str)
{
    size_t buf_size = str.size() * 2 + 2;
    std::vector<char> buf(buf_size);
    size_t write_len = ::WideCharToMultiByte(0, 0, str.data(), static_cast<int>(str.size()), buf.data(), static_cast<int>(buf.size()), nullptr, nullptr);
    return std::string(buf.data(), buf.data() + write_len);
}

Win32Error::Win32Error(DWORD error, const std::string& what) :
    std::system_error(error, std::system_category(), what)
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

void ThrowIfZero(const FILETIME& ft)
{
    // zero'd FILETIME conversions will fail in UTC-n, now they will always fail.
    if (ft.dwHighDateTime == 0 && ft.dwLowDateTime == 0)
    {
        throw std::exception("FILETIME == 0!");
    }
}

FILETIME FileTimeToLocalFileTime(const FILETIME& ft)
{
    ThrowIfZero(ft);
    FILETIME ftLocal;
    if (::FileTimeToLocalFileTime(&ft, &ftLocal) == 0)
    {
        ThrowLastError("FileTimeToLocalFileTime");
    }
    return ftLocal;
}

FILETIME LocalFileTimeToFileTime(const FILETIME& ftLocal)
{
    ThrowIfZero(ftLocal);
    FILETIME ft;
    if (::LocalFileTimeToFileTime(&ftLocal, &ft) == 0)
    {
        ThrowLastError("LocalFileTimeToFileTime");
    }
    return ft;
}

SYSTEMTIME FileTimeToSystemTime(const FILETIME& ft)
{
    ThrowIfZero(ft);
    SYSTEMTIME st;
    if (::FileTimeToSystemTime(&ft, &st) == 0)
    {
        ThrowLastError("FileTimeToSystemTime");
    }
    return st;
}

FILETIME SystemTimeToFileTime(const SYSTEMTIME& st)
{
    FILETIME ft;
    if (::SystemTimeToFileTime(&st, &ft) == 0)
    {
        ThrowLastError("SystemTimeToFileTime");
    }
    return ft;
}

Handle CreateFileMapping(HANDLE hFile, const SECURITY_ATTRIBUTES* pAttributes, DWORD protect, DWORD maximumSizeHigh, DWORD maximumSizeLow, const wchar_t* pName)
{
    Handle hMap(::CreateFileMappingW(hFile, const_cast<SECURITY_ATTRIBUTES*>(pAttributes), protect, maximumSizeHigh, maximumSizeLow, pName));
    if (!hMap)
    {
        ThrowLastError("CreateFileMapping");
    }

    return hMap;
}

Handle OpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId)
{
    Handle hProcessHandle(::OpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId));
    if (!hProcessHandle)
    {
        ThrowLastError(L"OpenProcess");
    }

    return hProcessHandle;
}

Handle CreateEvent(const SECURITY_ATTRIBUTES* pEventAttributes, bool manualReset, bool initialState, const wchar_t* pName)
{
    Handle hEvent(::CreateEventW(const_cast<SECURITY_ATTRIBUTES*>(pEventAttributes), static_cast<BOOL>(manualReset), static_cast<BOOL>(initialState), pName));
    if (!hEvent)
    {
        ThrowLastError("CreateEvent");
    }

    return hEvent;
}

void SetEvent(Handle& hEvent)
{
    SetEvent(hEvent.get());
}

void SetEvent(HANDLE hEvent)
{
    if (::SetEvent(hEvent) == 0)
    {
        ThrowLastError("SetEvent");
    }
}

Handle CreateMutex(const SECURITY_ATTRIBUTES* pMutexAttributes, bool initialOwner, const wchar_t* pName)
{
    Handle hMutex(::CreateMutexW(const_cast<SECURITY_ATTRIBUTES*>(pMutexAttributes), static_cast<BOOL>(initialOwner), pName));
    if (!hMutex)
    {
        ThrowLastError("CreateMutex");
    }

    return hMutex;
}

void SetSecurityInfo(HANDLE hObject, SE_OBJECT_TYPE ObjectType, SECURITY_INFORMATION SecurityInfo, const PSID psidOwner, const PSID psidGroup, const PACL pDacl, const PACL pSacl)
{
    if (::SetSecurityInfo(hObject, ObjectType, SecurityInfo, psidOwner, psidGroup, pDacl, pSacl) != ERROR_SUCCESS)
    {
        ThrowLastError("SetSecurityInfo");
    }
}

#ifdef DACL_MODIFY

//delete DACL at all, so permit Full Access for Everyone
void DeleteObjectDACL(HANDLE hObject)
{
    Win32::SetSecurityInfo(hObject, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, nullptr, nullptr);
}

//add necessary permissions for "Authenticated Users" group (all non-anonymous users)
void AdjustObjectDACL(HANDLE hObject)
{
    ACL* pOldDACL;
    SECURITY_DESCRIPTOR* pSD = nullptr;
    GetSecurityInfo(hObject, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &pOldDACL, nullptr, reinterpret_cast<void**>(&pSD));

    PSID pSid = nullptr;
    SID_IDENTIFIER_AUTHORITY authNt = SECURITY_NT_AUTHORITY;
    AllocateAndInitializeSid(&authNt, 1, SECURITY_AUTHENTICATED_USER_RID, 0, 0, 0, 0, 0, 0, 0, &pSid);

    EXPLICIT_ACCESS ea = {};
    ea.grfAccessMode = GRANT_ACCESS;
    ea.grfAccessPermissions = GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE;
    ea.grfInheritance = NO_INHERITANCE;
    ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea.Trustee.ptstrName = static_cast<LPTSTR>(pSid);

    ACL* pNewDACL = nullptr;
    SetEntriesInAcl(1, &ea, pOldDACL, &pNewDACL);

    Win32::SetSecurityInfo(hObject, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, pNewDACL, nullptr);

    FreeSid(pSid);
    LocalFree(pNewDACL);
    LocalFree(pSD);
}

#endif

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

void WaitForSingleObject(const Handle& hObject)
{
    WaitForSingleObject(hObject.get());
}

bool WaitForSingleObject(const Handle& hObject, DWORD milliSeconds)
{
    return WaitForSingleObject(hObject.get(), milliSeconds);
}

WaitResult::WaitResult(bool signaled, int index) :
    signaled(signaled),
    index(index)
{
}

WaitResult WaitForMultipleObjects(const HANDLE* begin, const HANDLE* end, bool waitAll, DWORD milliSeconds)
{
    size_t count = end - begin;
    auto rc = ::WaitForMultipleObjects(static_cast<DWORD>(count), begin, static_cast<BOOL>(waitAll), milliSeconds);
    if (rc == WAIT_TIMEOUT)
    {
        return WaitResult(false);
    }
    if (rc == WAIT_FAILED)
    {
        ThrowLastError("WaitForMultipleObjects");
    }
    if (rc >= WAIT_OBJECT_0 && rc < WAIT_OBJECT_0 + count)
    {
        return WaitResult(true, rc - WAIT_OBJECT_0);
    }
    {
        throw std::runtime_error("WaitForMultipleObjects");
    }
}

WaitResult WaitForAnyObject(const HANDLE* begin, const HANDLE* end, DWORD milliSeconds)
{
    return WaitForMultipleObjects(begin, end, false, milliSeconds);
}

WaitResult WaitForAllObjects(const HANDLE* begin, const HANDLE* end, DWORD milliSeconds)
{
    return WaitForMultipleObjects(begin, end, true, milliSeconds);
}

bool IsProcessRunning(HANDLE handle)
{
    return (::WaitForSingleObject(handle, 0) == WAIT_TIMEOUT);
}

MutexLock::MutexLock(HANDLE hMutex) :
    m_hMutex(hMutex)
{
    if (hMutex != nullptr)
    {
        WaitForSingleObject(hMutex);
    }
}

MutexLock::~MutexLock()
{
    if (m_hMutex != nullptr)
    {
        ReleaseMutex(m_hMutex);
    }
}

void MutexLock::Release()
{
    if (m_hMutex == nullptr)
    {
        return;
    }

    if (ReleaseMutex(m_hMutex) == 0)
    {
        ThrowLastError("ReleaseMutex");
    }
    m_hMutex = nullptr;
}


MappedViewOfFile::MappedViewOfFile(HANDLE hFileMappingObject, DWORD access, DWORD offsetHigh, DWORD offsetLow, size_t bytesToMap) :
    m_ptr(::MapViewOfFile(hFileMappingObject, access, offsetHigh, offsetLow, bytesToMap))
{
    if (m_ptr == nullptr)
    {
        ThrowLastError("MapViewOfFile");
    }
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

ScopedCursor::ScopedCursor(HCURSOR hCursor) :
    m_hCursor(::SetCursor(hCursor))
{
}

ScopedCursor::ScopedCursor(ScopedCursor&& sc) noexcept :
    m_hCursor(sc.m_hCursor)
{
    sc.m_hCursor = nullptr;
}

ScopedCursor::~ScopedCursor()
{
    if (m_hCursor != nullptr)
    {
        ::SetCursor(m_hCursor);
    }
}

void SetPrivilege(HANDLE hToken, const wchar_t* privilege, bool enablePrivilege)
{
    TOKEN_PRIVILEGES tp;
    LUID luid;
    if (!LookupPrivilegeValue(nullptr, privilege, &luid))
    {
        ThrowLastError("LookupPrivilegeValue");
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = enablePrivilege ? SE_PRIVILEGE_ENABLED : 0;

    if (AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr) == 0)
    {
        ThrowLastError("AdjustTokenPrivileges");
    }
}

void SetPrivilege(const wchar_t* privilege, bool enablePrivilege)
{
    HANDLE handle;
    if (::OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &handle) == 0)
    {
        ThrowLastError("OpenProcessToken");
    }

    Handle hToken(handle);
    SetPrivilege(hToken.get(), privilege, enablePrivilege);
}

// this retrieves the GetParentProcessId on platforms that support it
DWORD GetParentProcessId()
{
    ULONG_PTR pbi[6];
    ULONG ulSize = 0;
    long(WINAPI * NtQueryInformationProcess)(HANDLE ProcessHandle, ULONG ProcessInformationClass, void* ProcessInformation, ULONG ProcessInformationLength, ULONG* pReturnLength);
    *reinterpret_cast<FARPROC*>(&NtQueryInformationProcess) = GetProcAddress(LoadLibraryA("NTDLL.DLL"), "NtQueryInformationProcess");
    if ((NtQueryInformationProcess != nullptr) && NtQueryInformationProcess(GetCurrentProcess(), 0, &pbi, sizeof(pbi), &ulSize) >= 0 && ulSize == sizeof(pbi))
    {
        return static_cast<DWORD>(pbi[5]);
    }
    return static_cast<DWORD>(-1);
}

std::vector<std::wstring> GetCommandLineArguments()
{
    int argc = 0;
    HLocal args(CommandLineToArgvW(GetCommandLineW(), &argc));
    auto argv = static_cast<const wchar_t**>(args.get());
    return std::vector<std::wstring>(argv, argv + argc);
}

DWORD GetExitCodeProcess(HANDLE hProcess)
{
    DWORD exitCode;
    if (::GetExitCodeProcess(hProcess, &exitCode) == 0)
    {
        ThrowLastError("GetExitCodeProcess");
    }
    return exitCode;
}

DWORD GetExitCodeProcess(const Handle& hProcess)
{
    return GetExitCodeProcess(hProcess.get());
}

std::wstring GetWindowText(HWND hWnd)
{
    int length = ::GetWindowTextLengthW(hWnd);
    std::vector<wchar_t> text(length + 1);
    if (::GetWindowTextW(hWnd, text.data(), length + 1) == 0)
    {
        if (auto err = GetLastError())
        {
            throw Win32Error(err, "GetWindowText");
        }
    }
    return std::wstring(text.data(), length);
}

std::wstring GetDlgItemText(HWND hDlg, int idc)
{
    HWND hWnd = ::GetDlgItem(hDlg, idc);
    if (hWnd == nullptr)
    {
        ThrowLastError("GetExitCodeProcess");
    }
    return GetWindowText(hWnd);
}

bool IsGUIThread()
{
    return ::IsGUIThread(FALSE) == TRUE;
}

HFile::HFile(const std::string& filename) :
    m_handle(-1)
{
    if (::_sopen_s(&m_handle, filename.c_str(), _O_RDWR | _O_CREAT, _SH_DENYNO, _S_IREAD | _S_IWRITE) != 0)
    {
        ThrowLastError("_sopen_s");
    }
}

HFile::~HFile()
{
    if (m_handle != -1)
    {
        ::_close(m_handle);
    }
}

size_t HFile::size() const
{
    auto size = ::_filelength(m_handle);
    if (size == -1)
    {
        ThrowLastError("_filelength");
    }
    return size;
}

void HFile::resize(size_t size) const
{
    if (_chsize_s(m_handle, size) != 0)
    {
        ThrowLastError("_chsize");
    }
}

// clang-format off

// SEH (Structured Exception Handling) return codes are in the 0xC000000-0xfffff00 range
std::wstring GetSEHcodeDescription(DWORD code)
{
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:         return L"EXCEPTION_ACCESS_VIOLATION";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return L"EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_BREAKPOINT:               return L"EXCEPTION_BREAKPOINT";
    case EXCEPTION_DATATYPE_MISALIGNMENT:    return L"EXCEPTION_DATATYPE_MISALIGNMENT";
    case EXCEPTION_FLT_DENORMAL_OPERAND:     return L"EXCEPTION_FLT_DENORMAL_OPERAND";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return L"EXCEPTION_FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_FLT_INEXACT_RESULT:       return L"EXCEPTION_FLT_INEXACT_RESULT";
    case EXCEPTION_FLT_INVALID_OPERATION:    return L"EXCEPTION_FLT_INVALID_OPERATION";
    case EXCEPTION_FLT_OVERFLOW:             return L"EXCEPTION_FLT_OVERFLOW";
    case EXCEPTION_FLT_STACK_CHECK:          return L"EXCEPTION_FLT_STACK_CHECK";
    case EXCEPTION_FLT_UNDERFLOW:            return L"EXCEPTION_FLT_UNDERFLOW";
    case EXCEPTION_ILLEGAL_INSTRUCTION:      return L"EXCEPTION_ILLEGAL_INSTRUCTION";
    case EXCEPTION_IN_PAGE_ERROR:            return L"EXCEPTION_IN_PAGE_ERROR";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:       return L"EXCEPTION_INT_DIVIDE_BY_ZERO";
    case EXCEPTION_INT_OVERFLOW:             return L"EXCEPTION_INT_OVERFLOW";
    case EXCEPTION_INVALID_DISPOSITION:      return L"EXCEPTION_INVALID_DISPOSITION";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION: return L"EXCEPTION_NONCONTINUABLE_EXCEPTION";
    case EXCEPTION_PRIV_INSTRUCTION:         return L"EXCEPTION_PRIVILEGED_INSTRUCTION";
    case EXCEPTION_SINGLE_STEP:              return L"EXCEPTION_SINGLE_STEP";
    case EXCEPTION_STACK_OVERFLOW:           return L"EXCEPTION_STACK_OVERFLOW";
    case EXCEPTION_GUARD_PAGE:               return L"EXCEPTION_GUARD_PAGE";
    case EXCEPTION_INVALID_HANDLE:           return L"EXCEPTION_INVALID_HANDLE";
    case STATUS_DLL_NOT_FOUND:               return L"EXCEPTION_DLL_NOT_FOUND";
    case STATUS_NO_MEMORY:                   return L"EXCEPTION_NO_MEMORY";
    case STATUS_ORDINAL_NOT_FOUND :          return L"EXCEPTION_ORDINAL_NOT_FOUND";
    case STATUS_ENTRYPOINT_NOT_FOUND:        return L"EXCEPTION_ENTRYPOINT_NOT_FOUND";
    case STATUS_CONTROL_C_EXIT:              return L"EXCEPTION_CONTROL_C_EXIT"; 
    case STATUS_DLL_INIT_FAILED:             return L"DllMain returned false";
    case STATUS_FLOAT_MULTIPLE_FAULTS:       return L"EXCEPTION_FLOAT_MULTIPLE_FAULTS";
    case STATUS_FLOAT_MULTIPLE_TRAPS:        return L"EXCEPTION_FLOAT_MULTIPLE_TRAPS";
    case STATUS_REG_NAT_CONSUMPTION:         return L"EXCEPTION_REG_NAT_CONSUMPTION";
    case STATUS_STACK_BUFFER_OVERRUN:        return L"EXCEPTION_STACK_BUFFER_OVERRUN";
    case STATUS_INVALID_CRUNTIME_PARAMETER:  return L"EXCEPTION_INVALID_CRUNTIME_PARAMETER";  
    case STATUS_ASSERTION_FAILURE:           return L"EXCEPTION_ASSERTION_FAILURE";
    case DBG_EXCEPTION_HANDLED:              return L"DBG_EXCEPTION_HANDLED";
    case DBG_CONTINUE:                       return L"DBG_CONTINUE";
    case STATUS_SEGMENT_NOTIFICATION:        return L"STATUS_SEGMENT_NOTIFICATION";
    case DBG_TERMINATE_THREAD:               return L"DBG_TERMINATE_THREAD";
    case DBG_TERMINATE_PROCESS:              return L"DBG_TERMINATE_PROCESS";
    case DBG_CONTROL_C:                      return L"DBG_CONTROL_C";
    case DBG_PRINTEXCEPTION_C:               return L"DBG_PRINTEXCEPTION_C";
    case DBG_RIPEXCEPTION:                   return L"DBG_RIPEXCEPTION";
    case DBG_CONTROL_BREAK:                  return L"DBG_CONTROL_BREAK";
    case DBG_COMMAND_EXCEPTION:              return L"DBG_COMMAND_EXCEPTION";

    // undocumented? but regularly seen codes
    case 0xC0000022:                         return L"executable or one of the dependant dlls do not have execute rights";
    default: return L"UNKNOWN EXCEPTION";
    }
}

// RESULT return codes are in the 0x8000000-0xbffffff range
std::wstring GetHresultName(HRESULT hr)
{
    switch (hr) {
    case E_FAIL:            return L"E_FAIL";
    case E_ACCESSDENIED:    return L"E_ACCESSDENIED";
    case E_ABORT:           return L"E_ABORT";
    case E_NOTIMPL:         return L"E_NOTIMPL";
    case E_OUTOFMEMORY:     return L"E_OUTOFMEMORY";
    case E_INVALIDARG:      return L"E_INVALIDARG";
    case E_NOINTERFACE:     return L"E_NOINTERFACE";
    case E_POINTER:         return L"E_POINTER";
    case E_HANDLE:          return L"E_HANDLE";
    case E_UNEXPECTED:      return L"E_UNEXPECTED";
    default: break;
    }
    return L"";
}

// clang-format on

std::wstring GetHresultMessage(HRESULT hr)
{
    _com_error err(hr);
    return err.ErrorMessage();
}

std::wstring GetHresultDescription(HRESULT hr)
{
    auto msg = GetHresultMessage(hr);
    auto name = GetHresultName(hr);
    if (name.empty())
    {
        return msg;
    }
    return name + L", " + msg;
}

JobObject::JobObject() :
    m_jobHandle(::CreateJobObject(nullptr, nullptr))
{
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {};

    jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (0 == ::SetInformationJobObject(m_jobHandle.get(), JobObjectExtendedLimitInformation, &jeli, sizeof(jeli)))
    {
        ThrowLastError("SetInformationJobObject");
    }
}

HANDLE JobObject::get() const
{
    return m_jobHandle.get();
}

void JobObject::AddProcessById(DWORD pid) const
{
    Handle processHandle(OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid));
    ::AssignProcessToJobObject(m_jobHandle.get(), processHandle.get());
}

void JobObject::AddProcessByHandle(HANDLE processHandle) const
{
    ::AssignProcessToJobObject(m_jobHandle.get(), processHandle);
}

Handle DuplicateHandle(HANDLE handle)
{
    HANDLE result;
    ::DuplicateHandle(GetCurrentProcess(), handle, GetCurrentProcess(), &result, 0, FALSE, DUPLICATE_SAME_ACCESS);
    return Handle(result);
}

} // namespace Win32

bool operator==(const FILETIME& ft1, const FILETIME& ft2)
{
    return ::CompareFileTime(&ft1, &ft2) == 0;
}

bool operator!=(const FILETIME& ft1, const FILETIME& ft2)
{
    return !(ft1 == ft2);
}

bool operator<(const FILETIME& ft1, const FILETIME& ft2)
{
    return ::CompareFileTime(&ft1, &ft2) < 0;
}

bool operator>=(const FILETIME& ft1, const FILETIME& ft2)
{
    return !(ft1 < ft2);
}

bool operator>(const FILETIME& ft1, const FILETIME& ft2)
{
    return ft2 < ft1;
}

bool operator<=(const FILETIME& ft1, const FILETIME& ft2)
{
    return !(ft1 > ft2);
}

} // namespace fusion
