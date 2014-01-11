// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <boost/algorithm/string.hpp>
#include "resource.h"
#include "Win32Lib.h"
#include "dbgstream.h"
#include "hstream.h"
#include "DBWinWriter.h"
#include "Utilities.h"
#include "MainFrame.h"

#include "Utilities.h"
#include "IndexedStorage.h"

CAppModule _Module;

namespace fusion {
namespace debugviewpp {

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

int ForwardMessagesFromPipe(HANDLE hPipe)
{
	DBWinWriter dbwin;
	DWORD pid = GetParentProcessId();

	hstream pipe(hPipe);
	std::string line;
	while (std::getline(pipe, line))
		dbwin.Write(pid, line);

	return 0;
}

int Run(const wchar_t* /*cmdLine*/, int cmdShow)
{
	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hFile = nullptr, hPipe = nullptr;
	switch (GetFileType(hStdIn))
	{
	case FILE_TYPE_DISK: hFile = hStdIn; break;
	case FILE_TYPE_PIPE: hPipe = hStdIn; break;
	}

	if (hPipe && IsDBWinViewerActive())
		return ForwardMessagesFromPipe(hPipe);

	MessageLoop theLoop(_Module);

	int argc;
	wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	wchar_t* fileName = nullptr;

	for (int i = 1; i < argc; ++i)
	{
		if (boost::iequals(argv[i], L"/min"))
		{
			cmdShow = SW_MINIMIZE;
		}
		else if (argv[i][0] != '/')
		{
			if (fileName)
				throw std::runtime_error("Duplicate filename");
			fileName = argv[i];
		}
	}

	fusion::debugviewpp::CMainFrame wndMain;
	if (wndMain.CreateEx() == nullptr)
		ThrowLastError(L"Main window creation failed!");

	wndMain.ShowWindow(cmdShow);
	if (fileName)
		wndMain.Load(fileName);
	else if (hFile)
		wndMain.Load(hFile);
	else if (hPipe)
		wndMain.CapturePipe(hPipe);

	return theLoop.Run();
}

} // namespace debugviewpp 
} // namespace fusion

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
try
{
	fusion::SetPrivilege(SE_DEBUG_NAME, true);
	fusion::SetPrivilege(SE_CREATE_GLOBAL_NAME, true);

	fusion::ComInitialization com(fusion::ComInitialization::ApartmentThreaded);

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(nullptr, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	HRESULT hRes = _Module.Init(nullptr, hInstance);
	ATLASSERT(SUCCEEDED(hRes));
	hRes;

	int nRet = fusion::debugviewpp::Run(lpstrCmdLine, nCmdShow);

	_Module.Term();

	return nRet;
}
catch (std::exception& e)
{
	MessageBoxA(nullptr, e.what(), "DebugView++ Error", MB_OK | MB_ICONERROR);
	return -1;
}
