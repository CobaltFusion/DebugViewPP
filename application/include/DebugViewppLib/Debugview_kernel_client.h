// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

void InstallKernelMessagesDriver();
void UninstallKernelMessagesDriver();

#include <windows.h>
#include <tchar.h>
#include <winioctl.h>
#include <winsvc.h>

#include <string>
#include <iostream>

// this is the dbgv.sys interface
#define FILE_DEVICE_DBGV 0x8305

#define DBGV_CAPTURE_KERNEL          CTL_CODE(FILE_DEVICE_DBGV, DWORD(0), METHOD_BUFFERED, FILE_ANY_ACCESS)     //0x00  //enable capture kernel
#define DBGV_UNCAPTURE_KERNEL        CTL_CODE(FILE_DEVICE_DBGV, DWORD(1), METHOD_BUFFERED, FILE_ANY_ACCESS)     //0x04  //
#define DBGV_CLEAR_DISPLAY           CTL_CODE(FILE_DEVICE_DBGV, DWORD(2), METHOD_BUFFERED, FILE_ANY_ACCESS)     //0x08  //clear display
#define DBGV_READ_LOG                CTL_CODE(FILE_DEVICE_DBGV, DWORD(3), METHOD_NEITHER, FILE_ANY_ACCESS)      //0x0f  //read kernel log
#define DBGV_SET_PASSTHROUGH         CTL_CODE(FILE_DEVICE_DBGV, DWORD(4), METHOD_BUFFERED, FILE_ANY_ACCESS)     //0x10  //enable passthrough
#define DBGV_UNSET_PASSTHROUGH       CTL_CODE(FILE_DEVICE_DBGV, DWORD(5), METHOD_BUFFERED, FILE_ANY_ACCESS)     //0x14  //
#define DBGV_IS_DRIVER_AVAILABLE     CTL_CODE(FILE_DEVICE_DBGV, DWORD(8), METHOD_BUFFERED, FILE_ANY_ACCESS)     //0x20  //test driver is valid or functional
#define DBGV_GET_DRIVER_VERSION      CTL_CODE(FILE_DEVICE_DBGV, DWORD(9), METHOD_BUFFERED, FILE_ANY_ACCESS)     //0x24  //driver version, 4.70 = 0x800
#define DBGV_SET_CARRIAGE_RETURN     CTL_CODE(FILE_DEVICE_DBGV, DWORD(0x0d), METHOD_BUFFERED, FILE_ANY_ACCESS)  //0x34  //force carriage return
#define DBGV_UNSET_CARRIAGE_RETURN   CTL_CODE(FILE_DEVICE_DBGV, DWORD(0x0e), METHOD_BUFFERED, FILE_ANY_ACCESS)  //0x38  //
#define DBGV_SET_VERBOSE_MESSAGES    CTL_CODE(FILE_DEVICE_DBGV, DWORD(0x0f), METHOD_BUFFERED, FILE_ANY_ACCESS)  //0x3C  //enable log verbose
#define DBGV_UNSET_VERBOSE_MESSAGES  CTL_CODE(FILE_DEVICE_DBGV, DWORD(0x10), METHOD_BUFFERED, FILE_ANY_ACCESS)  //0x40  //reset log verbose

inline std::string feature_to_string(DWORD value)
{
    switch (value)
    {
    case DBGV_CAPTURE_KERNEL: return "DBGV_CAPTURE_KERNEL";
    case DBGV_UNCAPTURE_KERNEL: return "DBGV_UNCAPTURE_KERNEL";
    case DBGV_CLEAR_DISPLAY: return "DBGV_CLEAR_DISPLAY";
    case DBGV_READ_LOG: return "DBGV_READ_LOG";
    case DBGV_SET_PASSTHROUGH: return "DBGV_SET_PASSTHROUGH";
    case DBGV_UNSET_PASSTHROUGH: return "DBGV_UNSET_PASSTHROUGH";
    case DBGV_IS_DRIVER_AVAILABLE: return "DBGV_IS_DRIVER_AVAILABLE";
    case DBGV_GET_DRIVER_VERSION: return "DBGV_GET_DRIVER_VERSION";
    case DBGV_SET_CARRIAGE_RETURN: return "DBGV_SET_CARRIAGE_RETURN";
    case DBGV_UNSET_CARRIAGE_RETURN: return "DBGV_UNSET_CARRIAGE_RETURN";
    case DBGV_SET_VERBOSE_MESSAGES: return "DBGV_SET_VERBOSE_MESSAGES";
    case DBGV_UNSET_VERBOSE_MESSAGES: return "DBGV_UNSET_VERBOSE_MESSAGES";
    default: break;
    }
    return std::to_string(value);
}

// suppress the non-standard use of zero-sized array in struct/union
#pragma warning(disable:4200)

#pragma pack(1)
typedef struct
{
    DWORD dwIndex;
    FILETIME liSystemTime;
    LARGE_INTEGER liPerfCounter;
    CHAR strData[0];
} LOG_ITEM, *PLOG_ITEM;
#pragma pack()

constexpr const wchar_t * strDbgviewKernelDriverDeviceName = L"\\\\.\\dbgv";
constexpr const DWORD kernelMessageBufferSize = 0x10000;

