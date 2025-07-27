// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "MainFrame.h"

#include <boost/algorithm/string.hpp>
#include "CobaltFusion/scope_guard.h"
#include "CobaltFusion/hstream.h"
#include "CobaltFusion/fusionassert.h"
#include "DebugViewppLib/DBWinWriter.h"
#include "Win32/Com.h"

#include <iostream>

#include "atleverything.h"

//#define ENABLE_CRASHPAD
#ifdef ENABLE_CRASHPAD
#include "crashpad.h"
#endif

//#define CONSOLE_DEBUG

CAppModule _Module;

namespace fusion {
namespace debugviewpp {

class CAppModuleInitialization
{
public:
    CAppModuleInitialization(CAppModule& module, HINSTANCE hInstance) :
        m_module(module)
    {
        HRESULT hr = m_module.Init(nullptr, hInstance);
        if (FAILED(hr))
        {
            Win32::ThrowWin32Error(hr, "CAppModule::Init");
        }
    }

    ~CAppModuleInitialization()
    {
        m_module.Term();
    }

private:
    CAppModule& m_module;
};

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
    DWORD pid = Win32::GetParentProcessId();

    hstream pipe(hPipe);
    std::string line;
    while (std::getline(pipe, line))
    {
        line += "\n";
        dbwin.Write(pid, line);
    }

    return 0;
}

int Main(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpstrCmdLine*/, int cmdShow)
{
    Win32::SetPrivilege(SE_DEBUG_NAME, true);
    Win32::SetPrivilege(SE_CREATE_GLOBAL_NAME, true);

    Win32::ComInitialization com;

    // this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
    ::DefWindowProc(nullptr, 0, 0, 0L);

    AtlInitCommonControls(ICC_BAR_CLASSES); // add flags to support other controls

#ifdef CONSOLE_DEBUG
    FILE* standardOut;
    AllocConsole();
    freopen_s(&standardOut, "CONOUT$", "wb", stdout);
    auto fileGuard = make_guard([standardOut] { fclose(standardOut); });
    std::cout.clear();
#endif


    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_DBGV_DRIVER), RT_RCDATA);
    if (hRes)
    {
        HGLOBAL hLoadedRes = LoadResource(NULL, hRes);
        if (hLoadedRes)
        {
            DWORD dwSize = SizeofResource(NULL, hRes);
            void* pLockedRes = LockResource(hLoadedRes);
            if (pLockedRes)
            {
                std::ofstream outFile("dbgv.sys", std::ios::binary);
                outFile.write(static_cast<const char*>(pLockedRes), dwSize);
                outFile.close();
            }
        }
    }

    CAppModuleInitialization moduleInit(_Module, hInstance);

    HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hFile = nullptr;
    HANDLE hPipe = nullptr;
    switch (GetFileType(hStdIn))
    {
    case FILE_TYPE_DISK: hFile = hStdIn; break;
    case FILE_TYPE_PIPE: hPipe = hStdIn; break;
    default: break;
    }

    if ((hPipe != nullptr) && IsDBWinViewerActive())
    {
        return ForwardMessagesFromPipe(hPipe);
    }

    CMainFrame wndMain;
    MessageLoop theLoop(_Module);

    auto args = Win32::GetCommandLineArguments();
    std::wstring fileName;

    for (size_t i = 1; i < args.size(); ++i)
    {
        if (boost::iequals(args[i], L"/min"))
        {
            cmdShow = SW_MINIMIZE;
        }
        else if (boost::iequals(args[i], L"/log"))
        {
            //wndMain.SetLogging();        // todo: implement: FileWriter needs to concurrently access m_logfile, it now causes a crash if DbgMsgSrc -1 is run
            // this should be replaced by the new streaming-to-disk feature we discussed.
        }
        else if (args[i][0] != '/')
        {
            if (!fileName.empty())
            {
                throw std::runtime_error("multiple filenames specified on commandline");
            }
            fileName = args[i];
        }
    }

    if (wndMain.CreateEx() == nullptr)
    {
        Win32::ThrowLastError(L"Main window creation failed!");
    }

    wndMain.ShowWindow(cmdShow == SW_SHOWDEFAULT ? wndMain.GetShowCommand() : cmdShow);
    if (boost::algorithm::iends_with(fileName, ".dbconf"))
    {
        wndMain.LoadConfiguration(fileName);
    }
    else if (!fileName.empty())
    {
        wndMain.Load(fileName, false);
    }
    else if (hFile != nullptr)
    {
        wndMain.Load(hFile);
    }
    else if (hPipe != nullptr)
    {
        wndMain.CapturePipe(hPipe);
    }

    return theLoop.Run();
}

} // namespace debugviewpp
} // namespace fusion


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpstrCmdLine, int nCmdShow) try
{
#ifdef ENABLE_CRASHPAD
    initializeCrashPad();
#endif
    return fusion::debugviewpp::Main(hInstance, hPrevInstance, lpstrCmdLine, nCmdShow);
}
catch (std::exception& ex)
{
    fusion::errormessage(ex.what(), "Debugview++ Error");
    return EXIT_FAILURE;
}
