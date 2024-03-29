// PropertyControlTest.cpp : main source file for PropertyControlTest.exe
//

// #include "stdafx.h"


#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlctrlw.h>

CAppModule _Module;

#include "resource.h"
#include "maindlg.h"


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpstrCmdLine*/, int /*nCmdShow*/)
{
	HRESULT hRes = ::CoInitialize(NULL);
	ATLASSERT(SUCCEEDED(hRes));

	AtlInitCommonControls(ICC_WIN95_CLASSES | ICC_DATE_CLASSES);

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	CMainDlg dlgMain;
	int nRet = dlgMain.DoModal();

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
