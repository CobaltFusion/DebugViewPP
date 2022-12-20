// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <atlbase.h>
#include <atlwin.h>

#define DECLARE_MSG_MAP() \
    template <typename T, typename E1, typename E2, typename E3, typename E4, typename E5, typename E6, typename E7, typename E8, typename E9, typename E10> \
    friend struct ExceptionHandler; \
     \
    virtual BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID); \
    BOOL ProcessWindowMessageImpl2(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID);

// Alternative to ATL standard BEGIN_MSG_MAP() with try block:
#define BEGIN_MSG_MAP2(theClass) \
    BOOL theClass::ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID) \
    try \
    { \
        return ProcessWindowMessageImpl(hWnd, uMsg, wParam, lParam, lResult, dwMsgMapID); \
    } \
    catch (...) \
    { \
        OnException(); \
        return FALSE; \
    } \
     \
    BOOL theClass::ProcessWindowMessageImpl2(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID) \
    { \
        BOOL bHandled = TRUE; \
        (hWnd); \
        (uMsg); \
        (wParam); \
        (lParam); \
        (lResult); \
        (bHandled); \
        switch(dwMsgMapID) \
        { \
        case 0:

template <typename T, typename E1 = void, typename E2 = void, typename E3 = void, typename E4 = void, typename E5 = void, typename E6 = void, typename E7 = void, typename E8 = void, typename E9 = void, typename E10 = void>
struct ExceptionHandler : ExceptionHandler<T, E2, E3, E4, E5, E6, E7, E8, E9, E10>
{
    BOOL ProcessWindowMessageImpl(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID)
    try
    {
        return ExceptionHandler<T, E2, E3, E4, E5, E6, E7, E8, E9, E10>::ProcessWindowMessageImpl(hWnd, uMsg, wParam, lParam, lResult, dwMsgMapID);
    }
    catch (E1& ex)
    {
        static_cast<T*>(this)->OnException(ex);
        return FALSE;
    }
};

template <typename T>
struct ExceptionHandler<T>
{
    BOOL ProcessWindowMessageImpl(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID)
    {
        return static_cast<T*>(this)->ProcessWindowMessageImpl2(hWnd, uMsg, wParam, lParam, lResult, dwMsgMapID);
    }
};
