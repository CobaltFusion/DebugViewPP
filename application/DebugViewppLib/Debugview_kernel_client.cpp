// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <winioctl.h>
#include <winsvc.h>

#include <string>
#include <iostream>

#pragma warning(disable:4200)

#define FILE_DEVICE_DBGV 0x8305

#define DBGV_CAPTURE_KERNEL			CTL_CODE(FILE_DEVICE_DBGV, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)		//0x00	//enable capture kernel
#define DBGV_UNCAPTURE_KERNEL		CTL_CODE(FILE_DEVICE_DBGV, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)		//0x04	//
#define DBGV_CLEAR_DISPLAY			CTL_CODE(FILE_DEVICE_DBGV, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)		//0x08	//clear display
#define DBGV_READ_LOG				CTL_CODE(FILE_DEVICE_DBGV, 3, METHOD_NEITHER, FILE_ANY_ACCESS)		//0x0f	//read kernel log
#define DBGV_SET_PASSTHROUGH		CTL_CODE(FILE_DEVICE_DBGV, 4, METHOD_BUFFERED, FILE_ANY_ACCESS)		//0x10	//enable passthrough
#define DBGV_UNSET_PASSTHROUGH		CTL_CODE(FILE_DEVICE_DBGV, 5, METHOD_BUFFERED, FILE_ANY_ACCESS)		//0x14	//
#define DBGV_IS_DRIVER_AVAILABLE	CTL_CODE(FILE_DEVICE_DBGV, 8, METHOD_BUFFERED, FILE_ANY_ACCESS)		//0x20	//test driver is valid or functional
#define DBGV_GET_DRIVER_VERSION		CTL_CODE(FILE_DEVICE_DBGV, 9, METHOD_BUFFERED, FILE_ANY_ACCESS)		//0x24	//driver version, 4.70 = 0x800
#define DBGV_SET_CARRIAGE_RETURN	CTL_CODE(FILE_DEVICE_DBGV, 0x0d, METHOD_BUFFERED, FILE_ANY_ACCESS)	//0x34	//force carriage return
#define DBGV_UNSET_CARRIAGE_RETURN	CTL_CODE(FILE_DEVICE_DBGV, 0x0e, METHOD_BUFFERED, FILE_ANY_ACCESS)	//0x38	//
#define DBGV_ENABLE_FILTER_STATE	CTL_CODE(FILE_DEVICE_DBGV, 0x0f, METHOD_BUFFERED, FILE_ANY_ACCESS)	//0x3C	//enable log verbose
#define DBGV_SET_FILTER_STATE		CTL_CODE(FILE_DEVICE_DBGV, 0x10, METHOD_BUFFERED, FILE_ANY_ACCESS)	//0x40	//reset log verbose

#pragma pack(1)
typedef struct
{
	DWORD dwIndex;
	FILETIME liSystemTime;
	LARGE_INTEGER liPerfCounter;
	CHAR strData[0];
}LOG_ITEM, *PLOG_ITEM;
#pragma pack()

int monitor_kernel()
{
	std::wstring strDeviceName = L"\\\\.\\dbgv";

	HANDLE hFile = CreateFile(strDeviceName.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	DWORD dwErr = ::GetLastError();
	if (hFile != INVALID_HANDLE_VALUE)
	{
		BOOL bRet = FALSE;

		//enable capture
		DWORD dwOut = 0;
		DWORD dwReturned = 0;
		bRet = DeviceIoControl(hFile, DBGV_CAPTURE_KERNEL, NULL, 0, &dwOut, sizeof(dwOut), &dwReturned, NULL);
		if (!bRet)
		{
			printf("DBGV_CAPTURE_KERNEL failed, err=%d\n", ::GetLastError());
			CloseHandle(hFile);
			return -1;
		}

		//enable kernel verboase log
		bRet = DeviceIoControl(hFile, DBGV_ENABLE_FILTER_STATE, NULL, 0, NULL, 0, NULL, NULL);
		if (!bRet)
		{
			printf("DBGV_ENABLE_FILTER_STATE failed, err=%d\n", ::GetLastError());
			CloseHandle(hFile);
			return -2;
		}

		//try capture 1000 logs and exit
		const DWORD dwBufLen = 0x10000;
		PLOG_ITEM pBuf = (PLOG_ITEM)malloc(dwBufLen);
		DWORD nCount = 0, nMaxCount = 1000;
		while (1)
		{
			memset(pBuf, 0, dwBufLen);
			dwOut = 0;
			bRet = DeviceIoControl(hFile, DBGV_READ_LOG, NULL, 0, pBuf, dwBufLen, &dwOut, NULL);
			if (dwOut > 0)
			{
				PLOG_ITEM pNextItem = pBuf;
				while (pNextItem->dwIndex != 0)
				{
					SYSTEMTIME st = { 0 };
					FILETIME lt = { 0 };
					FileTimeToLocalFileTime(&pNextItem->liSystemTime, &lt);
					FileTimeToSystemTime(&lt, &st);
					printf("%d, Time:%04d-%02d-%02d %02d:%02d:%02d.%03d, %s\n",
						pNextItem->dwIndex,
						st.wYear,
						st.wMonth,
						st.wDay,
						st.wHour,
						st.wMinute,
						st.wSecond,
						st.wMilliseconds,
						pNextItem->strData);
					pNextItem = (PLOG_ITEM)((char*)pNextItem + sizeof(LOG_ITEM) + (strlen(pNextItem->strData) + 4) / 4 * 4);

					nCount++;
					if (nCount > nMaxCount)
					{
						break;
					}
				}
			}

			::Sleep(10);
		}

		::free(pBuf);

		bRet = DeviceIoControl(hFile, DBGV_UNCAPTURE_KERNEL, NULL, 0, NULL, 0, NULL, NULL);
		if (!bRet)
		{
			printf("DBGV_UNCAPTURE_KERNEL failed, err=%d\n", ::GetLastError());
			CloseHandle(hFile);
			return -1;
		}

		CloseHandle(hFile);
	}

	return 0;
};

/// Driver

constexpr const char* DRIVER_SERVICE_NAME = "debugviewdriver";
constexpr const char* DRIVER_DISPLAY_NAME = "DebugViewPP Kernel Message Driver";
const std::string driverPath = "dbgv.sys";

void InstallDriver()
{
    std::cout << "InstallDriver...\n";
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCManager) {
        std::cout << "Failed to open Service Control Manager. Error: " << GetLastError() << std::endl;
    }

    SC_HANDLE hService = CreateServiceA(
        hSCManager,
        DRIVER_SERVICE_NAME,
        DRIVER_DISPLAY_NAME,
        SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        driverPath.c_str(),
        NULL, NULL, NULL, NULL, NULL
    );

    if (!hService) {
        if (GetLastError() == ERROR_SERVICE_EXISTS) {
            std::cout << "Service already exists.\n";
            CloseServiceHandle(hSCManager);
        }
        std::cout << "Failed to create service. Error: " << GetLastError() << std::endl;
        CloseServiceHandle(hSCManager);
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    std::cout << "InstallDriver done...\n";
}

void UninstallDriver()
{
    std::cout << "UninstallDriver...\n";
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCManager) {
        std::cout << "Failed to open Service Control Manager. Error: " << GetLastError() << std::endl;
    }

    SC_HANDLE hService = OpenServiceA(hSCManager, DRIVER_SERVICE_NAME, DELETE);
    if (!hService) {
        std::cout << "Failed to open service. Error: " << GetLastError() << std::endl;
        CloseServiceHandle(hSCManager);
    }

    if (!DeleteService(hService)) {
        std::cout << "Failed to delete service. Error: " << GetLastError() << std::endl;
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    std::cout << "UninstallDriver done...\n";
}
