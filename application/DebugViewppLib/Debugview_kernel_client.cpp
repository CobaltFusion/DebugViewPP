// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "DebugViewppLib/Debugview_kernel_client.h"

constexpr const char* DRIVER_SERVICE_NAME = "debugviewdriver";
constexpr const char* DRIVER_DISPLAY_NAME = "DbgView Kernel Message Driver";
const std::string driverPath = "C:\\Windows\\System32\\drivers\\dbgvpp.sys";

bool FileExists(const std::string& path)
{
    DWORD fileAttributes = GetFileAttributesA(path.c_str());
    return (fileAttributes != INVALID_FILE_ATTRIBUTES &&
            !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY));
}

bool StartDriverSvc()
{
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCManager)
        return false;

    SC_HANDLE hService = OpenServiceA(hSCManager, DRIVER_SERVICE_NAME, SERVICE_START);
    if (!hService)
    {
        CloseServiceHandle(hSCManager);
        return false;
    }

    if (!StartService(hService, 0, NULL))
    {
        if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
        {
            std::cout << "Service is already running.\n";
            CloseServiceHandle(hService);
            CloseServiceHandle(hSCManager);
            return true;
        }
        std::cout << "Failed to start service. Error: " << GetLastError() << std::endl;
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
        return false;
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return true;
}

bool StopDriverSvc()
{
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCManager)
        return false;

    SC_HANDLE hService = OpenServiceA(hSCManager, DRIVER_SERVICE_NAME, SERVICE_STOP);
    if (!hService)
    {
        CloseServiceHandle(hSCManager);
        return false;
    }

    SERVICE_STATUS status;
    if (!ControlService(hService, SERVICE_CONTROL_STOP, &status))
    {
        DWORD err = GetLastError();
        if (err != ERROR_SERVICE_NOT_ACTIVE)
        {
            std::cout << "Failed to stop service. Error: " << err << std::endl;
        }
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return true;
}

void InstallKernelMessagesDriver(const std::string& driverLocation)
{
    // try to uninstall first, in case the driver is somehow still loaded.
    UninstallKernelMessagesDriver();

    if (!FileExists(driverPath))
    {
        std::cout << "Driver file not found at: " << driverPath << std::endl;
    }

    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCManager)
    {
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
        driverLocation.c_str(),
        NULL, NULL, NULL, NULL, NULL);

    if (!hService)
    {
        if (GetLastError() == ERROR_SERVICE_EXISTS)
        {
            std::cout << "Service already exists.\n";
            CloseServiceHandle(hSCManager);
        }
        std::cout << "Failed to create service. Error: " << GetLastError() << std::endl;
        CloseServiceHandle(hSCManager);
    }
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    StartDriverSvc();
}

void UninstallKernelMessagesDriver()
{
    StopDriverSvc();

    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCManager)
    {
        std::cout << "Failed to open Service Control Manager for uninstall. Error: " << GetLastError() << std::endl;
        return;
    }

    SC_HANDLE hService = OpenServiceA(hSCManager, DRIVER_SERVICE_NAME, DELETE);
    if (!hService)
    {
        // this happens when the service is first loaded, because we always uninstall before installing
        // std::cout << "Failed to open service for uninstall. Error: " << GetLastError() << std::endl;
        CloseServiceHandle(hSCManager);
        return;
    }

    if (!DeleteService(hService))
    {
        std::cout << "Failed to delete service. Error: " << GetLastError() << std::endl;
    }
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
}
