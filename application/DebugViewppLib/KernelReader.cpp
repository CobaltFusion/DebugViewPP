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

    // enable verbose kernel messages
    bRet = DeviceIoControl(handle.get(), DBGV_ENABLE_FILTER_STATE, NULL, 0, NULL, 0, NULL, NULL);
    if (!bRet)
    {
        printf("DBGV_ENABLE_FILTER_STATE failed, err=%d\n", ::GetLastError());
        return;
    }
    m_handle = std::move(handle);
    m_pBuf = (PLOG_ITEM)malloc(dwBufLen);
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
    AddMessage(0, "kernel", "Started capturing kernel messages");
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
    memset(m_pBuf, 0, dwBufLen);
    DWORD dwOut = 0;
    ::DeviceIoControl(m_handle.get(), DBGV_READ_LOG, NULL, 0, m_pBuf, dwBufLen, &dwOut, NULL);
    if (dwOut == 0) return; // no messages to be read

    PLOG_ITEM pNextItem = m_pBuf;
    while (pNextItem->dwIndex != 0)
    {
        SYSTEMTIME st = { 0 };
        FILETIME lt = { 0 };
        FileTimeToLocalFileTime(&pNextItem->liSystemTime, &lt);
        FileTimeToSystemTime(&lt, &st);

        char message[4000];
        sprintf(message, "%d, Time:%04d-%02d-%02d %02d:%02d:%02d.%03d, %s\n",
            pNextItem->dwIndex,
            st.wYear,
            st.wMonth,
            st.wDay,
            st.wHour,
            st.wMinute,
            st.wSecond,
            st.wMilliseconds,
            pNextItem->strData);

        AddMessage(0, "kernel", message);
        pNextItem = (PLOG_ITEM)((char*)pNextItem + sizeof(LOG_ITEM) + (strlen(pNextItem->strData) + 4) / 4 * 4);
    }
}

} // namespace debugviewpp
} // namespace fusion
