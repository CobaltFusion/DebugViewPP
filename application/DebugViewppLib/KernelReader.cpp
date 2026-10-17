// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "CobaltFusion/Str.h"
#include "DebugViewppLib/PolledLogSource.h"
#include "DebugViewppLib/KernelReader.h"
#include "DebugViewppLib/LineBuffer.h"
#include "DebugViewppLib/Debugview_kernel_client.h"

#include <chrono>
#include <filesystem>
#include <format>
#include <thread>

#include <mmsystem.h> // timeBeginPeriod / timeEndPeriod

namespace fusion {
namespace debugviewpp {

std::string GetDebugviewDriverLocation()
{
    const auto driver_name = "dbgvpp.sys";
    // this has issues, because there could already be an existing file
    // in that case we would load a possibly different driver verion?
    // an we also do not seem to be able to (always?) remove it ?
    // windows something reports it is 'in-use' even through we unloaded it.

    std::filesystem::path driverLocation = driver_name;
    const auto systemRoot = std::getenv("SystemRoot");
    if (systemRoot != nullptr)
    {
        driverLocation = std::format("{}\\System32\\drivers\\{}", systemRoot, driver_name);
    }
    return driverLocation.string();
}

void KernelReader::StartListening()
{
    Win32::Handle handle;
    DWORD lastError = ERROR_SUCCESS;
    for (int attempt = 0; attempt < 20; ++attempt)
    {
        handle.reset(::CreateFile(strDbgviewKernelDriverDeviceName,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL));
        if (handle.get() != INVALID_HANDLE_VALUE && handle.get() != nullptr)
        {
            break;
        }

        lastError = ::GetLastError();
        handle.reset();
        if (IsDbgviewDeviceBusyError(lastError) || !IsDbgviewDeviceMissingError(lastError))
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (handle.get() == INVALID_HANDLE_VALUE || handle.get() == nullptr)
    {
        printf("Could not open the kernel messages driver device, err=%d\n", lastError);
        if (IsDbgviewDeviceBusyError(lastError))
        {
            throw std::runtime_error(std::format("Kernel messages driver is already in use by another process. Error: {}", lastError));
        }
        if (IsDbgviewDeviceMissingError(lastError))
        {
            throw std::runtime_error(std::format("Could not find the kernel messages driver device name. Error: {}", lastError));
        }
        throw std::runtime_error(std::format("Could not open the kernel messages driver device. Error: {}", lastError));
    }

    DWORD dwReturned = 0;

    // flush any stale events left over from a previous session
    ::DeviceIoControl(handle.get(), DBGV_CLEAR_DISPLAY, NULL, 0, NULL, 0, &dwReturned, NULL);

    DWORD dwOut = 0;
    BOOL bRet = ::DeviceIoControl(handle.get(), DBGV_CAPTURE_KERNEL, NULL, 0, &dwOut, sizeof(dwOut), &dwReturned, NULL);
    if (!bRet)
    {
        const auto error = ::GetLastError();
        printf("DBGV_CAPTURE_KERNEL failed, err=%d\n", error);
        throw std::runtime_error(std::format("Could not enable capturing kernel messages. Error: {}", error));
    }

    m_handle = std::move(handle);
    m_pBuf = reinterpret_cast<PLOG_ITEM>(malloc(kernelMessageBufferSize));
    if (m_pBuf == nullptr)
    {
        StopListening();
        throw std::runtime_error("Could not allocate kernel message buffer");
    }
}

void KernelReader::StopListening()
{
    if (m_handle.get() == nullptr || m_handle.get() == INVALID_HANDLE_VALUE)
    {
        return;
    }

    // clear verbose / pass-through bits so the driver is left in a clean state for the next load
    ::DeviceIoControl(m_handle.get(), DBGV_UNSET_VERBOSE_MESSAGES, NULL, 0, NULL, 0, NULL, NULL);
    ::DeviceIoControl(m_handle.get(), DBGV_UNSET_PASSTHROUGH, NULL, 0, NULL, 0, NULL, NULL);

    BOOL bRet = ::DeviceIoControl(m_handle.get(), DBGV_UNCAPTURE_KERNEL, NULL, 0, NULL, 0, NULL, NULL);
    if (!bRet)
    {
        printf("DBGV_UNCAPTURE_KERNEL failed, err=%d\n", ::GetLastError());
    }
    m_handle.reset();
    ::free(m_pBuf);
    m_pBuf = nullptr;
}

KernelReader::KernelReader(Timer& timer, ILineBuffer& linebuffer) :
    PolledLogSource(timer, SourceType::Pipe, linebuffer, 1000) // 1ms poll
{
    SetDescription(L"Kernel Message Reader");
    bool timerPeriodRaised = false;
    try
    {
        InstallKernelMessagesDriver(GetDebugviewDriverLocation());
        Signal();
        StartListening();
        // bump the system timer resolution so the base class sleep_for(1ms) is actually ~1ms
        // instead of being rounded up to the default Windows scheduler tick
        ::timeBeginPeriod(1);
        timerPeriodRaised = true;
        StartThread();
    }
    catch (...)
    {
        StopListening();
        if (timerPeriodRaised)
        {
            ::timeEndPeriod(1);
        }
        UninstallKernelMessagesDriver();
        std::error_code ec;
        std::filesystem::remove(GetDebugviewDriverLocation(), ec);
        throw;
    }
}

KernelReader::~KernelReader()
{
    PolledLogSource::Abort();
    StopListening();
    ::timeEndPeriod(1);
    UninstallKernelMessagesDriver();
    std::error_code ec;
    std::filesystem::remove(GetDebugviewDriverLocation(), ec);
}

void KernelReader::Abort()
{
    AddMessage(0, "kernel", "<kernel message reader aborted>");
    Signal();
    PolledLogSource::Abort();
}

bool KernelReader::AtEnd() const
{
    return false;
}

void KernelReader::Poll()
{
    if (m_handle.get() == nullptr || m_handle.get() == INVALID_HANDLE_VALUE || m_pBuf == nullptr)
    {
        return;
    }

    // drain the driver ring until empty (bounded in time); a single IOCTL per poll was dropping
    // messages whenever the producer outran the poll rate
    const auto drainStarted = std::chrono::steady_clock::now();
    constexpr auto maxDrainTime = std::chrono::milliseconds(300);

    for (;;)
    {
        memset(m_pBuf, 0, kernelMessageBufferSize);
        DWORD dwOut = 0;
        if (!::DeviceIoControl(m_handle.get(), DBGV_READ_LOG, NULL, 0, m_pBuf, kernelMessageBufferSize, &dwOut, NULL) || dwOut == 0)
        {
            return; // no messages to be read
        }

        PLOG_ITEM pNextItem = m_pBuf;
        while (pNextItem->dwIndex != 0)
        {
            AddMessage(0, "kernel", pNextItem->strData);
            pNextItem = (PLOG_ITEM)((char*)pNextItem + sizeof(LOG_ITEM) + (strlen(pNextItem->strData) + 4) / 4 * 4);
        }

        if (std::chrono::steady_clock::now() - drainStarted >= maxDrainTime)
        {
            return;
        }
    }
}

void KernelReader::SetKernelMessagesDriverFeature(DWORD feature)
{
    DWORD dwOut = 0;
    DWORD dwReturned = 0;
    BOOL bRet = DeviceIoControl(m_handle.get(), feature, NULL, 0, &dwOut, sizeof(dwOut), &dwReturned, NULL);
    if (!bRet)
    {
        printf("SetKernelMessagesDriverFeature for '%s' failed, err=%d\n", feature_to_string(feature).c_str(), ::GetLastError());
        return;
    }
}

void KernelReader::SetVerbose(bool value)
{
    if (value)
    {
        SetKernelMessagesDriverFeature(DBGV_SET_VERBOSE_MESSAGES);
    }
    else
    {
        SetKernelMessagesDriverFeature(DBGV_UNSET_VERBOSE_MESSAGES);
    }
}

void KernelReader::SetPassThrough(bool value)
{
    if (value)
    {
        SetKernelMessagesDriverFeature(DBGV_SET_PASSTHROUGH);
    }
    else
    {
        SetKernelMessagesDriverFeature(DBGV_UNSET_PASSTHROUGH);
    }
}

} // namespace debugviewpp
} // namespace fusion
