// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "DebugViewppLib/Debugview_kernel_client.h"

#include <chrono>
#include <cstring>
#include <format>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{

constexpr const char* TempDriverRegistrySubKey = "System\\CurrentControlSet\\Services\\DebugViewPP_Dbgv";
constexpr const char* TempDriverNtRegistryPath = "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\DebugViewPP_Dbgv";
constexpr int DeviceRetryCount = 20;
constexpr auto DeviceRetryDelay = std::chrono::milliseconds(100);
using NtStatus = LONG;
constexpr NtStatus StatusImageAlreadyLoaded = static_cast<NtStatus>(0xC000010E);
constexpr NtStatus StatusObjectNameCollision = static_cast<NtStatus>(0xC0000035);

struct RtlAnsiString
{
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
};

struct RtlUnicodeString
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
};

using RtlAnsiStringToUnicodeStringFn = NtStatus(NTAPI*)(RtlUnicodeString*, RtlAnsiString*, BOOLEAN);
using RtlFreeUnicodeStringFn = VOID(NTAPI*)(RtlUnicodeString*);
using RtlNtStatusToDosErrorFn = ULONG(WINAPI*)(NtStatus);
using NtLoadDriverFn = NtStatus(NTAPI*)(RtlUnicodeString*);

std::string Win32ErrorMessage(const std::string& action, DWORD error)
{
    return action + ". Error: " + std::to_string(error);
}

bool FileExists(const std::string& path)
{
    const DWORD fileAttributes = GetFileAttributesA(path.c_str());
    return fileAttributes != INVALID_FILE_ATTRIBUTES &&
           !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

HMODULE GetNtdll()
{
    HMODULE module = GetModuleHandleA("ntdll.dll");
    if (module == nullptr)
    {
        module = LoadLibraryA("ntdll.dll");
    }
    if (module == nullptr)
    {
        throw std::runtime_error(Win32ErrorMessage("Failed to load ntdll.dll", GetLastError()));
    }
    return module;
}

template <typename T>
T GetNtdllProc(const char* name)
{
    const auto proc = reinterpret_cast<T>(GetProcAddress(GetNtdll(), name));
    if (proc == nullptr)
    {
        throw std::runtime_error(Win32ErrorMessage(std::format("Failed to resolve {}", name), GetLastError()));
    }
    return proc;
}

DWORD NtStatusToWin32(NtStatus status)
{
    try
    {
        return GetNtdllProc<RtlNtStatusToDosErrorFn>("RtlNtStatusToDosError")(status);
    }
    catch (...)
    {
        return static_cast<DWORD>(status);
    }
}

bool IsAcceptedLoadStatus(NtStatus status)
{
    return status >= 0 || status == StatusImageAlreadyLoaded || status == StatusObjectNameCollision;
}

bool EnablePrivilege(const char* privilegeName)
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
    {
        return false;
    }

    LUID luid = {};
    if (!LookupPrivilegeValueA(nullptr, privilegeName, &luid))
    {
        CloseHandle(token);
        return false;
    }

    TOKEN_PRIVILEGES privileges = {};
    privileges.PrivilegeCount = 1;
    privileges.Privileges[0].Luid = luid;
    privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    SetLastError(ERROR_SUCCESS);
    const BOOL adjusted = AdjustTokenPrivileges(token, FALSE, &privileges, sizeof(privileges), nullptr, nullptr);
    const DWORD error = GetLastError();
    CloseHandle(token);
    return adjusted && error == ERROR_SUCCESS;
}

std::string GetNtDriverPath(const std::string& driverLocation)
{
    if (driverLocation.rfind("\\??\\", 0) == 0)
    {
        return driverLocation;
    }
    return "\\??\\" + driverLocation;
}

void SetRegistryDword(HKEY key, const char* valueName, DWORD value)
{
    const auto result = RegSetValueExA(
        key,
        valueName,
        0,
        REG_DWORD,
        reinterpret_cast<const BYTE*>(&value),
        sizeof(value));
    if (result != ERROR_SUCCESS)
    {
        throw std::runtime_error(Win32ErrorMessage(std::format("Failed to set {}", valueName), result));
    }
}

void SetRegistryString(HKEY key, const char* valueName, const std::string& value)
{
    const auto result = RegSetValueExA(
        key,
        valueName,
        0,
        REG_SZ,
        reinterpret_cast<const BYTE*>(value.c_str()),
        static_cast<DWORD>(value.size() + 1));
    if (result != ERROR_SUCCESS)
    {
        throw std::runtime_error(Win32ErrorMessage(std::format("Failed to set {}", valueName), result));
    }
}

void DeleteTemporaryDriverRegistryKey()
{
    RegDeleteKeyA(HKEY_LOCAL_MACHINE, (std::string(TempDriverRegistrySubKey) + "\\Enum").c_str());
    RegDeleteKeyA(HKEY_LOCAL_MACHINE, (std::string(TempDriverRegistrySubKey) + "\\Security").c_str());
    RegDeleteKeyA(HKEY_LOCAL_MACHINE, TempDriverRegistrySubKey);
}

void CreateTemporaryDriverRegistryKey(const std::string& driverLocation)
{
    HKEY key = nullptr;
    const auto result = RegCreateKeyExA(
        HKEY_LOCAL_MACHINE,
        TempDriverRegistrySubKey,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE,
        nullptr,
        &key,
        nullptr);
    if (result != ERROR_SUCCESS)
    {
        throw std::runtime_error(Win32ErrorMessage("Failed to create temporary kernel driver registry key", result));
    }

    try
    {
        SetRegistryDword(key, "Type", SERVICE_KERNEL_DRIVER);
        SetRegistryDword(key, "ErrorControl", SERVICE_ERROR_NORMAL);
        SetRegistryDword(key, "Start", SERVICE_DEMAND_START);
        SetRegistryString(key, "ImagePath", GetNtDriverPath(driverLocation));
        SetRegistryString(key, "DisplayName", "DebugView++ temporary Dbgv driver");
    }
    catch (...)
    {
        RegCloseKey(key);
        DeleteTemporaryDriverRegistryKey();
        throw;
    }

    RegCloseKey(key);
}

bool TryOpenDbgviewDevice(DWORD& error)
{
    HANDLE device = CreateFileW(
        strDbgviewKernelDriverDeviceName,
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (device != INVALID_HANDLE_VALUE && device != nullptr)
    {
        CloseHandle(device);
        error = ERROR_SUCCESS;
        return true;
    }

    error = GetLastError();
    return false;
}

void WaitForDbgviewDevice()
{
    DWORD lastError = ERROR_SUCCESS;
    for (int attempt = 0; attempt < DeviceRetryCount; ++attempt)
    {
        if (TryOpenDbgviewDevice(lastError))
        {
            return;
        }

        if (IsDbgviewDeviceBusyError(lastError))
        {
            throw std::runtime_error(Win32ErrorMessage("Kernel messages driver is already in use by another process", lastError));
        }

        std::this_thread::sleep_for(DeviceRetryDelay);
    }

    throw std::runtime_error(Win32ErrorMessage("Could not open the kernel messages driver device", lastError));
}

void LoadDriverWithNtLoadDriver(const std::string& driverLocation)
{
    if (!EnablePrivilege("SeLoadDriverPrivilege"))
    {
        throw std::runtime_error(Win32ErrorMessage("Unable to enable SeLoadDriverPrivilege", GetLastError()));
    }

    CreateTemporaryDriverRegistryKey(driverLocation);

    NtStatus loadStatus = 0;
    try
    {
        RtlAnsiString ansiRegistryPath = {};
        ansiRegistryPath.Length = static_cast<USHORT>(strlen(TempDriverNtRegistryPath));
        ansiRegistryPath.MaximumLength = static_cast<USHORT>(ansiRegistryPath.Length + 1);
        ansiRegistryPath.Buffer = const_cast<PCHAR>(TempDriverNtRegistryPath);

        RtlUnicodeString unicodeRegistryPath = {};
        const auto rtlAnsiStringToUnicodeString = GetNtdllProc<RtlAnsiStringToUnicodeStringFn>("RtlAnsiStringToUnicodeString");
        const auto rtlFreeUnicodeString = GetNtdllProc<RtlFreeUnicodeStringFn>("RtlFreeUnicodeString");
        const auto ntLoadDriver = GetNtdllProc<NtLoadDriverFn>("NtLoadDriver");

        const NtStatus convertStatus = rtlAnsiStringToUnicodeString(&unicodeRegistryPath, &ansiRegistryPath, TRUE);
        if (convertStatus < 0)
        {
            throw std::runtime_error(std::format(
                "Failed to build kernel driver registry path. NTSTATUS: 0x{:08X}, Error: {}",
                static_cast<unsigned long>(convertStatus),
                NtStatusToWin32(convertStatus)));
        }

        loadStatus = ntLoadDriver(&unicodeRegistryPath);
        rtlFreeUnicodeString(&unicodeRegistryPath);
    }
    catch (...)
    {
        DeleteTemporaryDriverRegistryKey();
        throw;
    }

    DeleteTemporaryDriverRegistryKey();

    if (!IsAcceptedLoadStatus(loadStatus))
    {
        const DWORD error = NtStatusToWin32(loadStatus);
        throw std::runtime_error(std::format(
            "Failed to load kernel messages driver. NTSTATUS: 0x{:08X}, Error: {}",
            static_cast<unsigned long>(loadStatus),
            error));
    }
}

} // namespace

bool IsDbgviewDeviceMissingError(DWORD error)
{
    return error == ERROR_FILE_NOT_FOUND ||
           error == ERROR_PATH_NOT_FOUND ||
           error == ERROR_INVALID_NAME ||
           error == ERROR_BAD_PATHNAME;
}

bool IsDbgviewDeviceBusyError(DWORD error)
{
    return error == ERROR_ACCESS_DENIED ||
           error == ERROR_SHARING_VIOLATION ||
           error == ERROR_BUSY;
}

void InstallKernelMessagesDriver(const std::string& driverLocation)
{
    EnablePrivilege("SeDebugPrivilege");

    DWORD openError = ERROR_SUCCESS;
    if (TryOpenDbgviewDevice(openError))
    {
        return;
    }

    if (IsDbgviewDeviceBusyError(openError))
    {
        throw std::runtime_error(Win32ErrorMessage("Kernel messages driver is already in use by another process", openError));
    }

    if (!IsDbgviewDeviceMissingError(openError))
    {
        throw std::runtime_error(Win32ErrorMessage("Could not open the kernel messages driver device", openError));
    }

    if (!FileExists(driverLocation))
    {
        throw std::runtime_error("Kernel messages driver file not found at: " + driverLocation);
    }

    LoadDriverWithNtLoadDriver(driverLocation);
    WaitForDbgviewDevice();
}

void UninstallKernelMessagesDriver()
{
    DeleteTemporaryDriverRegistryKey();
}
