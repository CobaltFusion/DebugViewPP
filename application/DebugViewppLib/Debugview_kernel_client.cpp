// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "DebugViewppLib/Debugview_kernel_client.h"

constexpr const char* DRIVER_SERVICE_NAME = "debugviewdriver";
constexpr const char* DRIVER_DISPLAY_NAME = "DbgView Kernel Message Driver";
const std::string driverPath = "dbgv.sys";

void InstallKernelMessagesDriver()
{
    UninstallKernelMessagesDriver();
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
}

void UninstallKernelMessagesDriver()
{
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
}
