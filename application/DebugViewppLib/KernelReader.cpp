// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "CobaltFusion/Str.h"
#include "DebugViewppLib/PolledLogSource.h"
#include "DebugViewppLib/KernelReader.h"
#include "DebugViewppLib/LineBuffer.h"
#include "DebugViewppLib/Debugview_kernel_client.h"

namespace fusion {
namespace debugviewpp {

void KernelReader::StartListening()
{
    Win32::Handle handle(::CreateFile(strDbgviewKernelDriverDeviceName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL));
    if (handle.get() == INVALID_HANDLE_VALUE)
    {
        AddMessage(0, "internal", "Could not connected to kernel messages driver");
        return;
    }

    // enable capture
    DWORD dwOut = 0;
    DWORD dwReturned = 0;
    BOOL bRet = DeviceIoControl(handle.get(), DBGV_CAPTURE_KERNEL, NULL, 0, &dwOut, sizeof(dwOut), &dwReturned, NULL);
    if (!bRet)
    {
        printf("DBGV_CAPTURE_KERNEL failed, err=%d\n", ::GetLastError());
        return;
    }

    m_handle = std::move(handle);
    m_pBuf = reinterpret_cast<PLOG_ITEM>(malloc(kernelMessageBufferSize));
}

void KernelReader::StopListening()
{
    BOOL bRet = DeviceIoControl(m_handle.get(), DBGV_UNCAPTURE_KERNEL, NULL, 0, NULL, 0, NULL, NULL);
    if (!bRet)
    {
        printf("DBGV_UNCAPTURE_KERNEL failed, err=%d\n", ::GetLastError());
    }
    m_handle.reset();
    ::free(m_pBuf);
}

KernelReader::KernelReader(Timer& timer, ILineBuffer& linebuffer) :
    PolledLogSource(timer, SourceType::Pipe, linebuffer, 1)
{
    SetDescription(L"Kernel Message Reader");
    InstallKernelMessagesDriver();
    Signal();
    StartListening();
    StartThread();
}

KernelReader::~KernelReader()
{
    StopListening();
    UninstallKernelMessagesDriver();
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
    memset(m_pBuf, 0, kernelMessageBufferSize);
    DWORD dwOut = 0;
    ::DeviceIoControl(m_handle.get(), DBGV_READ_LOG, NULL, 0, m_pBuf, kernelMessageBufferSize, &dwOut, NULL);
    if (dwOut == 0) return; // no messages to be read

    PLOG_ITEM pNextItem = m_pBuf;
    while (pNextItem->dwIndex != 0)
    {
        AddMessage(0, "kernel", pNextItem->strData);
        pNextItem = (PLOG_ITEM)((char*)pNextItem + sizeof(LOG_ITEM) + (strlen(pNextItem->strData) + 4) / 4 * 4);
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
