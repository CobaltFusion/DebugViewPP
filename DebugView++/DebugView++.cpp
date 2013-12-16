// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include "resource.h"
#include "Win32Lib.h"
#include "MainFrm.h"

#include "Utilities.h"

CAppModule _Module;

class MessageLoop
{
public:
	explicit MessageLoop(CAppModule& module) :
		m_module(module)
	{
		module.AddMessageLoop(&m_loop);
	}

	~MessageLoop()
	{
		m_module.RemoveMessageLoop();
	}

	int Run()
	{
		return m_loop.Run();
	}

private:
	CAppModule& m_module;
	CMessageLoop m_loop;
};

int Run(LPTSTR lpstrCmdLine, int nCmdShow)
{
	MessageLoop theLoop(_Module);

	fusion::CMainFrame wndMain;
	if (wndMain.CreateEx() == nullptr)
	{
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}

	wndMain.ShowWindow(nCmdShow);

	return theLoop.Run();
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
try
{
	fusion::SetPrivilege(SE_DEBUG_NAME, TRUE);

	fusion::ComInitialization com(fusion::ComInitialization::ApartmentThreaded);

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(nullptr, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	HRESULT hRes = _Module.Init(nullptr, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();

	return nRet;
}
catch (std::exception& e)
{
	MessageBoxA(nullptr, e.what(), "DebugView++ Error", MB_OK | MB_ICONERROR);
	return -1;
}
