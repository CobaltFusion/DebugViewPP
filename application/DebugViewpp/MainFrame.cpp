// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <filesystem>
#include <optional>
#include <iostream>

#include <boost/optional.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "CobaltFusion/AtlWinExt.h"
#include "CobaltFusion/scope_guard.h"
#include "CobaltFusion/stringbuilder.h"
#include "CobaltFusion/dbgstream.h"
#include "CobaltFusion/hstream.h"
#include "CobaltFusion/Math.h"
#include "CobaltFusion/GuiExecutor.h"
#include "CobaltFusion/fusionassert.h"
#include "Win32/Registry.h"
#include "DebugViewppLib/ProcessReader.h"
#include "DebugViewppLib/DbgviewReader.h"
#include "DebugViewppLib/SocketReader.h"
#include "DebugViewppLib/FileReader.h"
#include "DebugViewppLib/FileIO.h"
#include "DebugViewppLib/LogFilter.h"

#include "resource.h"
#include "RunDlg.h"
#include "HistoryDlg.h"
#include "FilterDlg.h"
#include "SourcesDlg.h"
#include "AboutDlg.h"
#include "FileOptionDlg.h"
#include "LogView.h"
#include "MainFrame.h"

namespace fusion {
namespace debugviewpp {

using namespace std::chrono_literals;

std::wstring GetPersonalPath()
{
    std::wstring path;
    wchar_t szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_PERSONAL, nullptr, 0, szPath)))
    {
        path = szPath;
    }
    return path;
}

std::wstring FormatUnits(int n, const std::wstring& unit)
{
    if (n == 0)
    {
        return L"";
    }
    if (n == 1)
    {
        return wstringbuilder() << n << " " << unit;
    }
    return wstringbuilder() << n << " " << unit << "s";
}

std::wstring FormatDuration(double seconds)
{
    int minutes = FloorTo<int>(seconds / 60);
    seconds -= 60.0 * minutes;

    int hours = minutes / 60;
    minutes -= 60 * hours;

    int days = hours / 24;
    hours -= 24 * days;

    if (days > 0)
    {
        return wstringbuilder() << FormatUnits(days, L"day") << L" " << FormatUnits(hours, L"hour");
    }

    if (hours > 0)
    {
        return wstringbuilder() << FormatUnits(hours, L"hour") << L" " << FormatUnits(minutes, L"minute");
    }

    if (minutes > 0)
    {
        return wstringbuilder() << FormatUnits(minutes, L"minute") << L" " << FormatUnits(FloorTo<int>(seconds), L"second");
    }

    static const wchar_t* units[] = {L"s", L"ms", L"µs", L"ns", nullptr};
    const wchar_t** unit = units;
    while (*unit != nullptr && seconds > 0 && seconds < 1)
    {
        seconds *= 1e3;
        ++unit;
    }

    return wstringbuilder() << std::fixed << std::setprecision(3) << seconds << L" " << *unit;
}

BEGIN_MSG_MAP2(CMainFrame)
    MSG_WM_CREATE(OnCreate)
    MSG_WM_CLOSE(OnClose)
    MSG_WM_QUERYENDSESSION(OnQueryEndSession)
    MSG_WM_ENDSESSION(OnEndSession)
    MSG_WM_MOUSEWHEEL(OnMouseWheel)
    MSG_WM_CONTEXTMENU(OnContextMenu)
    MSG_WM_SYSCOMMAND(OnSysCommand)
    MESSAGE_HANDLER_EX(WM_SYSTEMTRAYICON, OnSystemTrayIcon)
    COMMAND_ID_HANDLER_EX(SC_RESTORE, OnScRestore)
    COMMAND_ID_HANDLER_EX(SC_CLOSE, OnScClose)
    COMMAND_ID_HANDLER_EX(ID_FILE_NEWVIEW, OnFileNewTab)
    COMMAND_ID_HANDLER_EX(ID_FILE_OPEN, OnFileOpen)
    COMMAND_ID_HANDLER_EX(ID_FILE_RUN, OnFileRun)
    COMMAND_ID_HANDLER_EX(ID_FILE_SAVE_LOG, OnFileSaveLog)
    COMMAND_ID_HANDLER_EX(ID_APP_EXIT, OnFileExit)
    COMMAND_ID_HANDLER_EX(ID_FILE_SAVE_VIEW, OnFileSaveView)
    COMMAND_ID_HANDLER_EX(ID_FILE_SAVE_VIEW_SELECTION, OnFileSaveViewSelection)
    COMMAND_ID_HANDLER_EX(ID_FILE_LOAD_CONFIGURATION, OnFileLoadConfiguration)
    COMMAND_ID_HANDLER_EX(ID_FILE_SAVE_CONFIGURATION, OnFileSaveConfiguration)
    COMMAND_ID_HANDLER_EX(ID_LOG_CLEAR, OnLogClear)
    COMMAND_ID_HANDLER_EX(ID_LOG_CROP, OnLogCrop)
    COMMAND_ID_HANDLER_EX(ID_LOG_PAUSE, OnLogPause)
    COMMAND_ID_HANDLER_EX(ID_LOG_GLOBAL, OnLogGlobal)
    COMMAND_ID_HANDLER_EX(ID_LOG_KERNEL, OnLogKernel)
    COMMAND_ID_HANDLER_EX(ID_LOG_KERNEL_VERBOSE, OnLogKernelVerbose)
    COMMAND_ID_HANDLER_EX(ID_LOG_KERNEL_PASSTHROUGH, OnLogKernelPassThrough)
    COMMAND_ID_HANDLER_EX(ID_LOG_HISTORY, OnLogHistory)
    COMMAND_ID_HANDLER_EX(ID_LOG_DEBUGVIEW_AGENT, OnLogDebugviewAgent)
    COMMAND_ID_HANDLER_EX(ID_VIEW_FIND, OnViewFind)
    COMMAND_ID_HANDLER_EX(ID_VIEW_FILTER, OnViewFilter)
    COMMAND_ID_HANDLER_EX(ID_VIEW_CLOSE, OnViewClose)
    COMMAND_ID_HANDLER_EX(ID_VIEW_DUPLICATE, OnViewDuplicate)
    COMMAND_ID_HANDLER_EX(ID_LOG_SOURCES, OnSources)
    COMMAND_ID_HANDLER_EX(ID_OPTIONS_LINKVIEWS, OnLinkViews)
    COMMAND_ID_HANDLER_EX(ID_OPTIONS_AUTONEWLINE, OnAutoNewline)
    COMMAND_ID_HANDLER_EX(ID_OPTIONS_PROCESS_PREFIX, OnProcessPrefix)
    COMMAND_ID_HANDLER_EX(ID_OPTIONS_FONT, OnViewFont)
    COMMAND_ID_HANDLER_EX(ID_OPTIONS_ALWAYSONTOP, OnAlwaysOnTop)
    COMMAND_ID_HANDLER_EX(ID_OPTIONS_HIDE, OnHide)
    COMMAND_ID_HANDLER_EX(ID_APP_ABOUT, OnAppAbout)
    NOTIFY_CODE_HANDLER_EX(CTCN_BEGINITEMDRAG, OnBeginTabDrag)
    NOTIFY_CODE_HANDLER_EX(CTCN_SELCHANGE, OnChangeTab)
    NOTIFY_CODE_HANDLER_EX(CTCN_CLOSE, OnCloseTab)
    NOTIFY_CODE_HANDLER_EX(CTCN_DELETEITEM, OnDeleteTab)

    if (uMsg == EM_REPLACESEL)
    {
        m_logSources.AddMessage(static_cast<DWORD>(wParam), "SendMessage", Str(reinterpret_cast<wchar_t*>(lParam)));
    }

    // forward WM_COMMANDs directly to the active LogView, otherwise TabbedFrame forwards WM_COMMANDs to the Splitter
    // which has FORWARD_NOTIFICATIONS() and sends it back the the CMainFrame causing an infinite loop
    if (uMsg == WM_COMMAND)
    {
        if (GetViewCount() > 0)
        {
            GetView().SendMessage(uMsg, wParam, lParam);
        }
        return TRUE;
    }
    CHAIN_MSG_MAP(TabbedFrame)
    CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
    REFLECT_NOTIFICATIONS()
END_MSG_MAP()

LOGFONT& GetDefaultLogFont()
{
    static LOGFONT lf;
    static int initialized = GetObjectW(AtlGetDefaultGuiFont(), sizeof(lf), &lf);
    return lf;
}

CMainFrame::CMainFrame() :
    m_findDlg(*this),
    m_tryGlobal(IsWindowsVistaOrGreater() && HasGlobalDBWinReaderRights()),
    m_logFileName(L"DebugView++.dblog"),
    m_txtFileName(L"Messages.dblog"),
    m_configFileName(L"DebugView++.dbconf"),
    m_initialPrivateBytes(ProcessInfo::GetPrivateBytes()),
    m_logfont(GetDefaultLogFont()),
    m_GuiExecutorClient(std::make_unique<GuiExecutorClient>()),
    m_logSources(*m_GuiExecutorClient)
{
    m_notifyIconData.cbSize = 0;
}

CMainFrame::~CMainFrame()
{
    RemoveDriver();
}

void CMainFrame::SetLogging()
{
    m_logWriter = std::make_unique<FileWriter>(L"debugview.dblog", m_logFile);
}

void CMainFrame::OnException()
{
    errormessage("No additional information available", "Exception occurred");
}

void CMainFrame::OnException(const std::exception& ex)
{
    errormessage(ex.what(), "Exception occurred");
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
    return TabbedFrame::PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle()
{
    UpdateUI();
    UIUpdateToolBar();
    UIUpdateStatusBar();
    return FALSE;
}

LRESULT CMainFrame::OnCreate(const CREATESTRUCT* /*pCreate*/)
{
    m_notifyIconData.cbSize = 0;

    HWND hWndCmdBar = m_cmdBar.Create(*this, rcDefault, nullptr, ATL_SIMPLE_CMDBAR_PANE_STYLE);
    m_cmdBar.AttachMenu(GetMenu());
    m_cmdBar.LoadImages(IDR_MAINFRAME);
    SetMenu(nullptr); //disable second menu bar

    CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
    CReBarCtrl rebar(m_hWndToolBar);

    AddSimpleReBarBand(hWndCmdBar);

    HWND hWndToolBar =
        CreateSimpleToolBarCtrl(rebar, IDR_MAINFRAME, 0, ATL_SIMPLE_TOOLBAR_PANE_STYLE); // DrMemory: LEAK 1696 direct bytes
    AddSimpleReBarBand(hWndToolBar, nullptr, 1);
    UIAddToolBar(hWndToolBar);

    m_findDlg.Create(rebar);
    AddSimpleReBarBand(m_findDlg, L"Find: ", 0, 10000);
    SizeSimpleReBarBands();

    rebar.LockBands(true);
    rebar.SetNotifyWnd(*this);

    m_hWndStatusBar = m_statusBar.Create(*this);
    int paneIds[] = {ID_DEFAULT_PANE, ID_SELECTION_PANE, ID_VIEW_PANE, ID_LOGFILE_PANE, ID_MEMORY_PANE};
    m_statusBar.SetPanes(paneIds, 5, false);
    UIAddStatusBar(m_hWndStatusBar);

    CreateTabWindow(*this, rcDefault, CTCS_CLOSEBUTTON | CTCS_DRAGREARRANGE);
    AddFilterView(L"View");
    HideTabControl();

    SetLogFont();
    LoadSettings();

    // register object for message filtering and idle updates
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != nullptr);
    pLoop->AddMessageFilter(this);
    pLoop->AddIdleHandler(this);

    m_logSources.SubscribeToUpdate([this] { return OnUpdate(); });

    // Resume can throw if a second debugview is running
    // so do not rely on any commands executed afterwards
    Resume();
    return 0;
}

void CMainFrame::OnClose()
{
    SaveSettings();
    DestroyWindow();

    if (m_notifyIconData.cbSize != 0u)
    {
        Shell_NotifyIcon(NIM_DELETE, &m_notifyIconData);
        m_notifyIconData.cbSize = 0;
    }
}

LRESULT CMainFrame::OnQueryEndSession(WPARAM /*unused*/, LPARAM /*unused*/)
{
    // MSDN:
    // The WM_QUERYENDSESSION message is sent when the user chooses to end the session or when an application calls one of the system
    // shutdown functions When an application returns TRUE for this message, it receives the WM_ENDSESSION message. Each application should
    // return TRUE or FALSE immediately upon receiving this message, and defer any cleanup operations until it receives the WM_ENDSESSION
    // message.
    return TRUE;
}

LRESULT CMainFrame::OnEndSession(WPARAM /*unused*/, LPARAM /*unused*/)
{
    OnClose();
    return TRUE;
}

void CMainFrame::UpdateUI()
{
    UpdateStatusBar();

    UISetCheck(ID_VIEW_TIME, GetView().GetClockTime());
    UISetCheck(ID_VIEW_PROCESSCOLORS, GetView().GetViewProcessColors());
    UISetCheck(ID_VIEW_SCROLL, GetView().GetAutoScroll());
    UISetCheck(ID_VIEW_SCROLL_STOP, GetView().GetAutoScrollStop());
    UISetCheck(ID_VIEW_BOOKMARK, GetView().GetBookmark());

    for (int id = ID_VIEW_COLUMN_FIRST; id <= ID_VIEW_COLUMN_LAST; ++id)
    {
        UISetCheck(id, GetView().IsColumnViewed(id));
    }

    UISetCheck(ID_OPTIONS_LINKVIEWS, m_linkViews);
    UIEnable(ID_OPTIONS_LINKVIEWS, static_cast<BOOL>(GetTabCtrl().GetItemCount() > 1));
    UISetCheck(ID_OPTIONS_AUTONEWLINE, m_logSources.GetAutoNewLine());
    UISetCheck(ID_OPTIONS_PROCESS_PREFIX, m_logSources.GetProcessPrefix());
    UISetCheck(ID_OPTIONS_ALWAYSONTOP, GetAlwaysOnTop());
    UISetCheck(ID_OPTIONS_HIDE, m_hide);
    UISetCheck(ID_LOG_PAUSE, !m_pLocalReader);
    UIEnable(ID_LOG_GLOBAL, !!m_pLocalReader);
    UISetCheck(ID_LOG_GLOBAL, m_tryGlobal);
    UISetCheck(ID_LOG_KERNEL, m_tryKernel);
    UIEnable(ID_LOG_KERNEL_VERBOSE, !!m_pKernelReader);
    UIEnable(ID_LOG_KERNEL_PASSTHROUGH, !!m_pKernelReader);
    UISetCheck(ID_LOG_KERNEL_VERBOSE, m_verboseKernelMessage);
    UISetCheck(ID_LOG_KERNEL_PASSTHROUGH, m_passthroughMode);
}

std::wstring FormatDateTime(const SYSTEMTIME& systemTime)
{
    int size = GetTimeFormat(LOCALE_USER_DEFAULT, 0, &systemTime, nullptr, nullptr, 0);
    size += GetDateFormat(LOCALE_USER_DEFAULT, 0, &systemTime, nullptr, nullptr, 0);
    std::vector<wchar_t> buf(size);

    int offset = GetDateFormat(LOCALE_USER_DEFAULT, 0, &systemTime, nullptr, buf.data(), size);
    buf[offset - 1] = ' ';
    GetTimeFormat(LOCALE_USER_DEFAULT, 0, &systemTime, nullptr, buf.data() + offset, size);
    return std::wstring(buf.data(), size - 1);
}

std::wstring FormatDateTime(const FILETIME& fileTime)
{
    return FormatDateTime(Win32::FileTimeToSystemTime(Win32::FileTimeToLocalFileTime(fileTime)));
}

std::wstring FormatBytes(size_t size)
{
    static const wchar_t* units[] = {L"bytes", L"kB", L"MB", L"GB", L"TB", L"PB", L"EB", nullptr};
    const wchar_t** unit = units;
    const int kb = 1024;
    while (size / kb > 0 && unit[1] != nullptr)
    {
        size = size / kb;
        ++unit;
    }

    return wstringbuilder() << size << L" " << *unit;
}

std::wstring CMainFrame::GetSelectionInfoText(const std::wstring& label, const SelectionInfo& selection) const
{
    if (selection.count == 0)
    {
        return std::wstring();
    }

    if (selection.count == 1)
    {
        return label + L": " + FormatDateTime(m_logFile[selection.beginLine].systemTime);
    }

    double dt = m_logFile[selection.endLine].time - m_logFile[selection.beginLine].time;
    return wstringbuilder() << label << L": " << FormatDuration(dt) << L" (" << selection.count << " lines)";
}

SelectionInfo CMainFrame::GetLogFileRange() const
{
    if (m_logFile.Empty())
    {
        return SelectionInfo();
    }

    return SelectionInfo(0, m_logFile.Count() - 1, m_logFile.Count());
}

void CMainFrame::UpdateStatusBar()
{
    auto isearch = GetView().GetHighlightText();
    std::wstring search = wstringbuilder() << L"Searching: \"" << isearch << L"\"";
    UISetText(ID_DEFAULT_PANE, isearch.empty() ? (m_pLocalReader != nullptr ? L"Ready" : L"Paused") : search.c_str());
    UISetText(ID_SELECTION_PANE, GetSelectionInfoText(L"Selected", GetView().GetSelectedRange()).c_str());
    UISetText(ID_VIEW_PANE, GetSelectionInfoText(L"View", GetView().GetViewRange()).c_str());
    UISetText(ID_LOGFILE_PANE, GetSelectionInfoText(L"Log", GetLogFileRange()).c_str());

    auto currentUsage = ProcessInfo::GetPrivateBytes();
    auto memoryUsage = currentUsage > m_initialPrivateBytes ? currentUsage - m_initialPrivateBytes : 0;
    UISetText(ID_MEMORY_PANE, FormatBytes(memoryUsage).c_str());
}

void CMainFrame::ProcessLines(const Lines& lines)
{
    if (lines.empty())
    {
        return;
    }

    // design decision: filtering is done on the UI thread, see CLogView::Add
    // changing this would introduces extra thread and thus complexity. Do that only if it solves a problem.

    int views = GetViewCount();
    for (int i = 0; i < views; ++i)
    {
        GetView(i).BeginUpdate();
    }

    if (m_logSources.GetProcessPrefix())
    {
        for (auto& line : lines)
        {
            AddMessage(Message(line.time, line.systemTime, line.pid, line.processName, "[" + std::to_string(line.pid) + "] " + line.message));
        }
    }
    else
    {
        for (auto& line : lines)
        {
            AddMessage(Message(line.time, line.systemTime, line.pid, line.processName, line.message));
        }
    }

    for (int i = 0; i < views; ++i)
    {
        if (GetView(i).EndUpdate() && GetTabCtrl().GetCurSel() != i)
        {
            SetModifiedMark(i, true);
            GetTabCtrl().UpdateLayout();
            GetTabCtrl().Invalidate();
        }
    }
}

bool CMainFrame::OnUpdate()
{
    Lines bucket;
    auto lines = m_logSources.GetLines();
    int count = 0;
    for (auto&& line : lines)
    {
        if (count++ < 5000)
        {
            bucket.emplace_back(std::move(line));
        }
        else
        {
            count = 0;
            m_incomingMessages.emplace_back(std::move(bucket));
            bucket = Lines();
        }
    }
    if (!bucket.empty())
    {
        m_incomingMessages.emplace_back(std::move(bucket));
    }

    if (m_incomingMessages.empty())
    {
        return false;
    }


    auto linesbucket = std::move(m_incomingMessages.front());
    m_incomingMessages.pop_front();
    ProcessLines(linesbucket);
    if (!m_incomingMessages.empty())
    {
        m_GuiExecutorClient->CallAfter(20ms, [this] { OnUpdate(); });
    }

    return true;
}

bool CMainFrame::OnMouseWheel(UINT nFlags, short zDelta, CPoint /*pt*/)
{
    if ((nFlags & MK_CONTROL) == 0)
    {
        return false;
    }

    int size = static_cast<int>(LogFontSizeToPointSize(m_logfont.lfHeight) * std::pow(1.15, zDelta / WHEEL_DELTA) + 0.5);
    size = std::max(size, 4);
    size = std::min(size, 24);
    m_logfont.lfHeight = LogFontSizeFromPointSize(size);
    SetLogFont();
    return true;
}

void CMainFrame::OnContextMenu(HWND hWnd, CPoint pt)
{
    if (hWnd != m_TabCtrl)
    {
        SetMsgHandled(win32::False);
        return;
    }

    CTCHITTESTINFO hit;
    hit.pt = pt;
    m_TabCtrl.ScreenToClient(&hit.pt);
    int item = m_TabCtrl.HitTest(&hit);

    CMenu menuContext;
    menuContext.LoadMenu(IDR_TAB_CONTEXTMENU);
    CMenuHandle menuPopup(menuContext.GetSubMenu(0));

    if (item < 0)
    {
        menuPopup.EnableMenuItem(ID_VIEW_FILTER, MF_BYCOMMAND | MF_GRAYED);
        menuPopup.EnableMenuItem(ID_VIEW_CLEAR, MF_BYCOMMAND | MF_GRAYED);
        menuPopup.EnableMenuItem(ID_VIEW_CLOSE, MF_BYCOMMAND | MF_GRAYED);
    }

    menuPopup.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, *this);
}

void CMainFrame::HandleDroppedFile(const std::wstring& file)
{
    SetTitle(file);
    using boost::algorithm::iequals;
    auto ext = std::filesystem::path(file).extension().wstring();
    if (iequals(ext, L".exe"))
    {
        Run(file);
    }
    else if (iequals(ext, L".cmd") || iequals(ext, L".bat"))
    {
        m_logSources.AddProcessReader(L"cmd.exe", wstringbuilder() << L"/Q /C \"" << file << "\"");
    }
    else
    {
        if (IsBinaryFileType(IdentifyFile(file)))
        {
            m_logSources.AddBinaryFileReader(file);
        }
        else
        {
            m_logSources.AddAnyFileReader(file, true);
        }
    }
}

void CMainFrame::OnDropped(const std::wstring uri)
{
    if (std::filesystem::is_regular_file(uri))
    {
        HandleDroppedFile(uri);
    }
    else
    {
        std::wstring httpmonitor = wstringbuilder() << Win32::GetExecutionPath() << "\\debugview++plugins\\HttpMonitor.exe";
        if (std::filesystem::exists(httpmonitor))
        {
            if (m_httpMonitorHandle)
            {
                ::TerminateProcess(m_httpMonitorHandle.get(), 0);
            }

            Win32::Process process(httpmonitor, uri);
            m_httpMonitorHandle = Win32::DuplicateHandle(process.GetProcessHandle());
            m_jobs.AddProcessByHandle(m_httpMonitorHandle.get());
        }
        else
        {
            m_logSources.AddMessage(stringbuilder() << httpmonitor << " missing, dropped url ignored\n");
        }
    }
}

LRESULT CMainFrame::OnSysCommand(UINT nCommand, CPoint /*unused*/)
{
    switch (nCommand)
    {
    case SC_MINIMIZE:
        if (!m_hide)
        {
            break;
        }

        if (m_notifyIconData.cbSize == 0u)
        {
            m_notifyIconData.cbSize = sizeof(m_notifyIconData);
            m_notifyIconData.hWnd = *this;
            m_notifyIconData.uID = 1;
            m_notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            m_notifyIconData.uCallbackMessage = WM_SYSTEMTRAYICON;
            m_notifyIconData.hIcon =
                AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
            CString sWindowText;
            GetWindowText(sWindowText);
            _tcscpy_s(m_notifyIconData.szTip, sWindowText);
            if (!Shell_NotifyIcon(NIM_ADD, &m_notifyIconData))
            {
                break;
            }
        }
        ShowWindow(SW_HIDE);
        return 0;
    default:
        break;
    }

    SetMsgHandled(win32::False);
    return 0;
}

LRESULT CMainFrame::OnSystemTrayIcon(UINT /*unused*/, WPARAM /*unused*/, LPARAM lParam)
{
    switch (lParam)
    {
    case WM_LBUTTONDBLCLK: SendMessage(WM_COMMAND, SC_RESTORE); break;
    case WM_RBUTTONUP: {
        SetForegroundWindow(m_hWnd);
        CMenuHandle menu = GetSystemMenu(0);
        menu.EnableMenuItem(SC_RESTORE, MF_BYCOMMAND | MF_ENABLED);
        menu.EnableMenuItem(SC_MOVE, MF_BYCOMMAND | MF_GRAYED);
        menu.EnableMenuItem(SC_SIZE, MF_BYCOMMAND | MF_GRAYED);
        menu.EnableMenuItem(SC_MINIMIZE, MF_BYCOMMAND | MF_GRAYED);
        menu.EnableMenuItem(SC_MAXIMIZE, MF_BYCOMMAND | MF_GRAYED);
        menu.EnableMenuItem(SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
        POINT position = Win32::GetCursorPos();
        menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN, position.x, position.y, m_hWnd);
        break;
    }
    default:
        break;
    }
    return 0;
}

LRESULT CMainFrame::OnScRestore(UINT /*unused*/, INT /*unused*/, HWND /*unused*/)
{
    if (m_notifyIconData.cbSize != 0u)
    {
        Shell_NotifyIcon(NIM_DELETE, &m_notifyIconData);
        m_notifyIconData.cbSize = 0;
    }
    ShowWindow(SW_SHOW);
    BringWindowToTop();
    return 0;
}

LRESULT CMainFrame::OnScClose(UINT /*unused*/, INT /*unused*/, HWND /*unused*/)
{
    PostMessage(WM_COMMAND, ID_APP_EXIT);
    return 0;
}

const wchar_t* RegistryPath = L"Software\\Cobalt Fusion\\DebugView++";

int CMainFrame::LogFontSizeFromPointSize(int fontSize)
{
    CDC dc(GetDC());
    return -MulDiv(fontSize, GetDeviceCaps(dc, LOGPIXELSY), 72);
}

int CMainFrame::LogFontSizeToPointSize(int logFontSize)
{
    CDC dc(GetDC());
    return -MulDiv(logFontSize, 72, GetDeviceCaps(dc, LOGPIXELSY));
}

bool CMainFrame::LoadSettings()
{
    auto mutex = Win32::CreateMutex(nullptr, false, L"Local\\DebugView++");
    Win32::MutexLock lock(mutex.get());

    DWORD x = 0;
    DWORD y = 0;
    DWORD cx = 0;
    DWORD cy = 0;
    CRegKey reg;
    reg.Create(HKEY_CURRENT_USER, RegistryPath);
    if (reg.QueryDWORDValue(L"X", x) == ERROR_SUCCESS &&
        reg.QueryDWORDValue(L"Y", y) == ERROR_SUCCESS &&
        reg.QueryDWORDValue(L"Width", cx) == ERROR_SUCCESS &&
        reg.QueryDWORDValue(L"Height", cy) == ERROR_SUCCESS)
    {
        WINDOWPLACEMENT placement = {0};
        placement.length = sizeof(placement);

        placement.rcNormalPosition.left = x;
        placement.rcNormalPosition.top = y;
        placement.rcNormalPosition.right = x + cx;
        placement.rcNormalPosition.bottom = y + cy;

        if (reg.QueryDWORDValue(L"MaxX", x) == ERROR_SUCCESS &&
            reg.QueryDWORDValue(L"MaxY", y) == ERROR_SUCCESS)
        {
            placement.ptMaxPosition.x = x;
            placement.ptMaxPosition.y = y;
        }

        // This is tricky: we should _not_ use the show command corresponding
        // to the actual initial window state in WINDOWPLACEMENT because doing
        // this loses the normal geometry, i.e. if we pass SW_SHOWMAXIMIZED to
        // SetWindowPlacement(), the window appears maximized initially, but
        // restoring it doesn't restore its previous rectangle.
        //
        // Instead, we always use SW_SHOWNORMAL here, but use the appropriate
        // show command for the ShowWindow() call in Main(), as this ensures
        // both that the window starts initially maximized, if it was closed in
        // this state, and that it can be restored to its last normal position.
        DWORD on = FALSE;
        if (reg.QueryDWORDValue(L"Maximized", on) == ERROR_SUCCESS && on)
            m_showCmd = SW_SHOWMAXIMIZED;
        else if (reg.QueryDWORDValue(L"Minimized", on) == ERROR_SUCCESS && on)
            m_showCmd = SW_SHOWMINIMIZED;

        placement.showCmd = SW_SHOWNORMAL;

        // Ignore errors, the worst that can happen is that the window doesn't
        // appear at the correct position, but this is not the end of the world.
        ::SetWindowPlacement(*this, &placement);
    }

    m_linkViews = Win32::RegGetDWORDValue(reg, L"LinkViews", 0) != 0;
    m_logSources.SetAutoNewLine(Win32::RegGetDWORDValue(reg, L"AutoNewLine", 1) != 0);
    SetAlwaysOnTop(Win32::RegGetDWORDValue(reg, L"AlwaysOnTop", 0) != 0);

    m_applicationName = Win32::RegGetStringValue(reg, L"ApplicationName", L"DebugView++");
    SetTitle();

    m_hide = Win32::RegGetDWORDValue(reg, L"Hide", 0) != 0;

    auto fontName = Win32::RegGetStringValue(reg, L"FontName", L"").substr(0, LF_FACESIZE - 1);
    int fontSize = Win32::RegGetDWORDValue(reg, L"FontSize", 8);
    if (!fontName.empty())
    {
        LOGFONT lf = {0};
        m_logfont = lf;
        std::copy(fontName.begin(), fontName.end(), m_logfont.lfFaceName);
        m_logfont.lfHeight = LogFontSizeFromPointSize(fontSize);
        SetLogFont();
    }

    CRegKey regViews;
    if (regViews.Open(reg, L"Views") == ERROR_SUCCESS)
    {
        for (size_t i = 0;; ++i)
        {
            CRegKey regView;
            if (regView.Open(regViews, WStr(wstringbuilder() << L"View" << i)) != ERROR_SUCCESS)
            {
                break;
            }

            auto name = Win32::RegGetStringValue(regView);
            if (i == 0)
            {
                GetTabCtrl().GetItem(0)->SetText(name.c_str());
            }
            else
            {
                AddFilterView(name);
            }
            GetView().LoadSettings(regView);
        }
        GetTabCtrl().SetCurSel(Win32::RegGetDWORDValue(regViews, L"Current", 0));
        GetTabCtrl().UpdateLayout();
        GetTabCtrl().Invalidate();
    }

    CRegKey regColors;
    if (regColors.Open(reg, L"Colors") == ERROR_SUCCESS)
    {
        auto colors = ColorDialog::GetCustomColors();
        for (int i = 0; i < 16; ++i)
        {
            colors[i] = Win32::RegGetDWORDValue(regColors, WStr(wstringbuilder() << L"Color" << i));
        }
    }

    return true;
}

void CMainFrame::SaveSettings()
{
    auto mutex = Win32::CreateMutex(nullptr, false, L"Local\\DebugView++");
    Win32::MutexLock lock(mutex.get());

    auto placement = Win32::GetWindowPlacement(*this);

    CRegKey reg;
    reg.Create(HKEY_CURRENT_USER, RegistryPath);
    reg.SetDWORDValue(L"X", placement.rcNormalPosition.left);
    reg.SetDWORDValue(L"Y", placement.rcNormalPosition.top);
    reg.SetDWORDValue(L"Width", placement.rcNormalPosition.right - placement.rcNormalPosition.left);
    reg.SetDWORDValue(L"Height", placement.rcNormalPosition.bottom - placement.rcNormalPosition.top);

    reg.SetDWORDValue(L"Maximized", placement.showCmd == SW_SHOWMAXIMIZED);
    reg.SetDWORDValue(L"Minimized", placement.showCmd == SW_SHOWMINIMIZED);

    reg.SetDWORDValue(L"MaxX", placement.ptMaxPosition.x);
    reg.SetDWORDValue(L"MaxY", placement.ptMaxPosition.y);

    reg.SetDWORDValue(L"LinkViews", static_cast<DWORD>(m_linkViews));
    reg.SetDWORDValue(L"AutoNewLine", static_cast<DWORD>(m_logSources.GetAutoNewLine()));
    reg.SetDWORDValue(L"AlwaysOnTop", static_cast<DWORD>(GetAlwaysOnTop()));
    reg.SetDWORDValue(L"Hide", static_cast<DWORD>(m_hide));

    reg.SetStringValue(L"FontName", m_logfont.lfFaceName);
    reg.SetDWORDValue(L"FontSize", LogFontSizeToPointSize(m_logfont.lfHeight));

    reg.RecurseDeleteKey(L"Views");

    CRegKey regViews;
    regViews.Create(reg, L"Views");
    regViews.SetDWORDValue(L"Current", GetTabCtrl().GetCurSel());

    int views = GetViewCount();
    for (int i = 0; i < views; ++i)
    {
        CRegKey regView;
        regView.Create(regViews, WStr(wstringbuilder() << L"View" << i));
        regView.SetStringValue(L"", GetView(i).GetName().c_str());
        GetView(i).SaveSettings(regView);
    }

    CRegKey regColors;
    regColors.Create(reg, L"Colors");
    auto colors = ColorDialog::GetCustomColors();
    for (int i = 0; i < 16; ++i)
    {
        regColors.SetDWORDValue(WStr(wstringbuilder() << L"Color" << i), colors[i]);
    }
}

void CMainFrame::FindNext(const std::wstring& text)
{
    if (!GetView().FindNext(text))
    {
        MessageBeep(MB_ICONASTERISK);
    }
}

void CMainFrame::FindPrevious(const std::wstring& text)
{
    if (!GetView().FindPrevious(text))
    {
        MessageBeep(MB_ICONASTERISK);
    }
}

void CMainFrame::AddFilterView()
{
    ++m_filterNr;
    CFilterDlg dlg(wstringbuilder() << L"View " << m_filterNr);
    if (dlg.DoModal() != IDOK)
    {
        return;
    }

    AddFilterView(dlg.GetName(), dlg.GetFilters());
    SaveSettings();
}

void CMainFrame::AddFilterView(const std::wstring& name, const LogFilter& filter)
{
    AddFilterView(std::make_shared<CLogView>(name, *this, m_logFile, filter));
}

void CMainFrame::AddFilterView(std::shared_ptr<CLogView> logview)
{
    auto pTabItem = std::make_unique<SelectedTabItem>();
    pTabItem->Create(*this);

    logview->Create(pTabItem->GetLogViewParent(), rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
    logview->SetFont(m_hFont.get());

    pTabItem->SetText(logview->GetName().c_str());
    pTabItem->SetView(logview);

    int newIndex = GetTabCtrl().GetItemCount();

    // notice: InsertItem takes ownership of the raw pointer, DeleteItem calls delete
    GetTabCtrl().InsertItem(newIndex, pTabItem.release());
    GetTabCtrl().SetCurSel(newIndex);
    ShowTabControl();
}

LRESULT CMainFrame::OnBeginTabDrag(NMHDR* pnmh)
{
    auto& nmhdr = *reinterpret_cast<NMCTCITEM*>(pnmh);

    return static_cast<LRESULT>(nmhdr.iItem >= GetViewCount());
}

LRESULT CMainFrame::OnChangeTab(NMHDR* pnmh)
{
    SetMsgHandled(win32::False);

    auto& nmhdr = *reinterpret_cast<NMCTC2ITEMS*>(pnmh);

    if (nmhdr.iItem2 >= 0 && nmhdr.iItem2 < GetViewCount())
    {
        SetModifiedMark(nmhdr.iItem2, false);
    }

    if (!m_linkViews || nmhdr.iItem1 == nmhdr.iItem2 || nmhdr.iItem1 < 0 || nmhdr.iItem1 >= GetViewCount() || nmhdr.iItem2 < 0 ||
        nmhdr.iItem2 >= GetViewCount())
    {
        return 0;
    }

    int line = GetView(nmhdr.iItem1).GetFocusLine();
    GetView(nmhdr.iItem2).SetFocusLine(line);

    return 0;
}

void CMainFrame::SetModifiedMark(int tabindex, bool modified)
{
    auto name = GetView(tabindex).GetName();
    if (modified)
    {
        name += L"*";
    }

    //    GetTabCtrl().GetItem(nmhdr.iItem2)->SetHighlighted(modified)
    GetTabCtrl().GetItem(tabindex)->SetText(name.c_str());
}

LRESULT CMainFrame::OnCloseTab(NMHDR* pnmh)
{
    auto& nmhdr = *reinterpret_cast<NMCTCITEM*>(pnmh);
    CloseView(nmhdr.iItem);
    return 0;
}

LRESULT CMainFrame::OnDeleteTab(NMHDR* pnmh)
{
    auto& nmhdr = *reinterpret_cast<NMCTCITEM*>(pnmh);

    if (nmhdr.iItem >= 0 && nmhdr.iItem < GetViewCount())
    {
        GetView(nmhdr.iItem).DestroyWindow();
    }

    return FALSE;
}

void CMainFrame::OnFileNewTab(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    AddFilterView();
}

void CMainFrame::SaveLogFile(const std::wstring& filename)
{
    UISetText(0, WStr(wstringbuilder() << "Saving " << filename));
    Win32::ScopedCursor cursor(::LoadCursor(nullptr, IDC_WAIT));

    std::ofstream fs;
    OpenLogFile(fs, filename);
    int count = m_logFile.Count();
    for (int i = 0; i < count; ++i)
    {
        auto msg = m_logFile[i];
        WriteLogFileMessage(fs, msg.time, msg.systemTime, msg.processId, msg.processName, msg.text);
    }
    fs.close();
    if (!fs)
    {
        Win32::ThrowLastError(filename);
    }

    m_logFileName = filename;
    UpdateStatusBar();
}

void CMainFrame::SaveViewFile(const std::wstring& filename)
{
    UISetText(0, WStr(wstringbuilder() << "Saving view to " << filename));
    Win32::ScopedCursor cursor(::LoadCursor(nullptr, IDC_WAIT));
    GetView().Save(filename);
    m_txtFileName = filename;
    UpdateStatusBar();
}

void CMainFrame::SaveViewSelection(const std::wstring& filename)
{
    UISetText(0, WStr(wstringbuilder() << "Saving selection to " << filename));
    Win32::ScopedCursor cursor(::LoadCursor(nullptr, IDC_WAIT));
    GetView().SaveSelection(filename);
    m_txtFileName = filename;
    UpdateStatusBar();
}
struct View
{
    int index;
    std::string name;
    bool clockTime;
    bool processColors;
    LogFilter filters;
    boost::optional<boost::property_tree::ptree> columnsPt;
};

struct SourceInfoHelper
{
    SourceInfoHelper(int index, const std::wstring& description, SourceType::type sourceType) :
        index(index),
        sourceInfo(description, sourceType)
    {
    }

    int index;
    SourceInfo sourceInfo;
};

void CMainFrame::LoadConfiguration(const std::wstring& fileName)
{
    boost::property_tree::ptree pt;
    boost::property_tree::read_xml(Str(fileName), pt, boost::property_tree::xml_parser::trim_whitespace);

    auto autoNewline = pt.get<bool>("DebugViewPP.AutoNewline");
    auto linkViews = pt.get<bool>("DebugViewPP.LinkViews");

    auto viewsPt = pt.get_child("DebugViewPP.Views");
    std::vector<View> views;
    for (auto& item : viewsPt)
    {
        if (item.first == "View")
        {
            View view;
            auto& viewPt = item.second;
            view.index = viewPt.get<int>("Index");
            view.name = viewPt.get<std::string>("Name");
            view.clockTime = viewPt.get<bool>("ClockTime");
            view.processColors = viewPt.get<bool>("ProcessColors");
            view.filters.messageFilters = MakeFilters(viewPt.get_child("MessageFilters"));
            view.filters.processFilters = MakeFilters(viewPt.get_child("ProcessFilters"));
            view.columnsPt = viewPt.get_child_optional("Columns");
            views.push_back(view);
        }
    }
    std::sort(views.begin(), views.end(), [](const View& v1, const View& v2) { return v1.index < v2.index; });

    m_logSources.SetAutoNewLine(autoNewline);
    m_linkViews = linkViews;

    for (int i = 0; i < static_cast<int>(views.size()); ++i)
    {
        if (i >= GetViewCount())
        {
            AddFilterView(WStr(views[i].name), views[i].filters);
        }
        else
        {
            GetView(i).SetFilters(views[i].filters);
        }

        auto& logView = GetView(i);
        logView.SetClockTime(views[i].clockTime);
        logView.SetViewProcessColors(views[i].processColors);
        if (views[i].columnsPt)
        {
            logView.ReadColumns(*(views[i].columnsPt));
        }
    }

    int i = GetViewCount();
    auto size = static_cast<int>(views.size());
    while (i > size)
    {
        --i;
        CloseView(i);
    }

    std::vector<SourceInfo> sourceInfos;
    auto sourcesPt = pt.get_child_optional("DebugViewPP.Sources");
    if (sourcesPt)
    {
        std::vector<SourceInfoHelper> sources;
        for (const auto& item : *sourcesPt)
        {
            if (item.first == "Source")
            {
                auto& sourcePt = item.second;
                int index = sourcePt.get<int>("Index");
                SourceType::type type = StringToSourceType(sourcePt.get<std::string>("SourceType"));
                std::wstring description = WStr(sourcePt.get<std::string>("Description"));
                SourceInfoHelper helper(index, description, type);
                helper.sourceInfo.address = WStr(sourcePt.get<std::string>("Address")).str();
                helper.sourceInfo.port = sourcePt.get<int>("Port");
                helper.sourceInfo.enabled = sourcePt.get<bool>("Enabled");
                sources.push_back(helper);
            }
        }

        std::sort(sources.begin(), sources.end(), [](const SourceInfoHelper& si1, const SourceInfoHelper& si2) { return si1.index < si2.index; });

        for (const auto& helper : sources)
        {
            sourceInfos.push_back(helper.sourceInfo);
        }
    }
    UpdateLogSources(sourceInfos);
    m_sourceInfos = sourceInfos;
}

void CMainFrame::SaveConfiguration(const std::wstring& fileName)
{
#if BOOST_VERSION < 105600
    boost::property_tree::xml_writer_settings<char> settings('\t', 1);
#else
    boost::property_tree::xml_writer_settings<std::string> settings('\t', 1);
#endif

    boost::property_tree::ptree mainPt;
    mainPt.put("AutoNewline", m_logSources.GetAutoNewLine());
    mainPt.put("LinkViews", m_linkViews);

    int views = GetViewCount();
    for (int i = 0; i < views; ++i)
    {
        auto& logView = GetView(i);
        auto filters = logView.GetFilters();
        boost::property_tree::ptree viewPt;
        viewPt.put("Index", i);
        viewPt.put("Name", Str(logView.GetName()).str());
        viewPt.put("ClockTime", logView.GetClockTime());
        viewPt.put("ProcessColors", logView.GetViewProcessColors());
        viewPt.put_child("MessageFilters", MakePTree(filters.messageFilters));
        viewPt.put_child("ProcessFilters", MakePTree(filters.processFilters));
        viewPt.put_child("Columns", MakePTree(logView.GetColumns()));
        mainPt.add_child("Views.View", viewPt);
    }

    for (int i = 0; i < static_cast<int>(m_sourceInfos.size()); ++i)
    {
        const auto& sourceInfo = m_sourceInfos[i];
        boost::property_tree::ptree sourcePt;
        sourcePt.put("Index", i);
        sourcePt.put("Enabled", sourceInfo.enabled);
        sourcePt.put("Description", Str(sourceInfo.description).str());
        sourcePt.put("SourceType", SourceTypeToString(sourceInfo.type));
        sourcePt.put("Address", Str(sourceInfo.address).str());
        sourcePt.put("Port", sourceInfo.port);
        mainPt.add_child("Sources.Source", sourcePt);
    }

    boost::property_tree::ptree pt;
    pt.add_child("DebugViewPP", mainPt);

    boost::property_tree::write_xml(Str(fileName), pt, std::locale(), settings);
}

void CMainFrame::OnFileOpen(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CFileOptionDlg dlg(1, L"Keep file open", L".dblog", m_logFileName.c_str(), OFN_FILEMUSTEXIST,
        L"DebugView++ Log Files (*.dblog)\0*.dblog\0"
        L"DebugView Log Files (*.log)\0*.log\0"
        L"All Files (*.*)\0*.*\0\0",
        nullptr);
    dlg.m_ofn.nFilterIndex = 0;
    dlg.m_ofn.lpstrTitle = L"Load Log File";
    if (dlg.DoModal() == IDOK)
    {
        Load(std::wstring(dlg.m_szFileName), dlg.Option());
    }
}

void CMainFrame::Run(const std::wstring& pathName)
{
    if (!pathName.empty())
    {
        m_runDlg.SetPathName(pathName);
    }

    if (m_runDlg.DoModal() == IDOK)
    {
        m_logSources.AddProcessReader(m_runDlg.GetPathName(), m_runDlg.GetArguments());
    }
}

void CMainFrame::OnFileRun(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    Run();
}

void CMainFrame::Load(const std::wstring& filename, bool keeptailing)
{
    SetTitle(filename);
    ClearLog();
    m_logSources.AddAnyFileReader(WStr(std::filesystem::path(filename).filename().string()), keeptailing);
}

void CMainFrame::SetTitle(const std::wstring& title)
{
    std::wstring windowText = title.empty() ? m_applicationName : L"[" + title + L"] - " + m_applicationName;
    SetWindowText(windowText.c_str());
}

void CMainFrame::Load(HANDLE hFile)
{
    hstream file(hFile);
    FILETIME ft = Win32::GetSystemTimeAsFileTime();
    Load(file, "", ft);
}

void CMainFrame::Load(std::istream& file, const std::string& name, FILETIME fileTime)
{
    Win32::ScopedCursor cursor(::LoadCursor(nullptr, IDC_WAIT));

    ClearLog();

    Line line(0.0);
    line.processName = name;
    line.systemTime = fileTime;
    while (ReadLogFileMessage(file, line))
    {
        AddMessage(Message(line.time, line.systemTime, line.pid, line.processName, line.message));
    }
}

void CMainFrame::CapturePipe(HANDLE hPipe)
{
    m_logSources.AddPipeReader(Win32::GetParentProcessId(), hPipe);
}

void CMainFrame::OnFileExit(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    PostMessage(WM_CLOSE);
}

void CMainFrame::OnFileSaveLog(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CFileDialog dlg(0, L".dblog", m_logFileName.c_str(), OFN_OVERWRITEPROMPT,
        L"DebugView++ Log Files (*.dblog)\0*.dblog\0"
        L"All Files (*.*)\0*.*\0\0",
        nullptr);
    dlg.m_ofn.nFilterIndex = 0;
    dlg.m_ofn.lpstrTitle = L"Save all messages in memory buffer";
    if (dlg.DoModal() == IDOK)
    {
        SaveLogFile(dlg.m_szFileName);
    }
}

void CMainFrame::OnFileSaveView(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CFileDialog dlg(0, L".dblog", m_txtFileName.c_str(), OFN_OVERWRITEPROMPT,
        L"DebugView++ Log Files (*.dblog)\0*.dblog\0"
        L"All Files (*.*)\0*.*\0\0");
    dlg.m_ofn.nFilterIndex = 0;
    dlg.m_ofn.lpstrTitle = L"Save the messages in the current view";
    if (dlg.DoModal() == IDOK)
    {
        SaveViewFile(dlg.m_szFileName);
    }
}

void CMainFrame::OnFileSaveViewSelection(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CFileDialog dlg(0, L".dblog", m_txtFileName.c_str(), OFN_OVERWRITEPROMPT,
        L"DebugView++ Log Files (*.dblog)\0*.dblog\0"
        L"All Files (*.*)\0*.*\0\0");
    dlg.m_ofn.nFilterIndex = 0;
    dlg.m_ofn.lpstrTitle = L"Save the messages selected in the current view";
    if (dlg.DoModal() == IDOK)
    {
        SaveViewSelection(dlg.m_szFileName);
    }
}

void CMainFrame::OnFileLoadConfiguration(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CFileDialog dlg(1, L".dbconf", m_configFileName.c_str(), OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
        L"DebugView++ Configuration Files (*.dbconf)\0*.dbconf\0\0");
    dlg.m_ofn.nFilterIndex = 0;
    dlg.m_ofn.lpstrTitle = L"Load View Configuration";
    if (dlg.DoModal() == IDOK)
    {
        LoadConfiguration(dlg.m_szFileName);
    }
}

void CMainFrame::OnFileSaveConfiguration(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CFileDialog dlg(
        0, L".dbconf", m_configFileName.c_str(), OFN_OVERWRITEPROMPT, L"DebugView++ Configuration Files (*.dbconf)\0*.dbconf\0\0");
    dlg.m_ofn.nFilterIndex = 0;
    dlg.m_ofn.lpstrTitle = L"Save View Configuration";
    if (dlg.DoModal() == IDOK)
    {
        SaveConfiguration(dlg.m_szFileName);
    }
}

void CMainFrame::ClearLog()
{
    m_logFile.Clear();
    m_logSources.ResetTimer();
    int views = GetViewCount();
    for (int i = 0; i < views; ++i)
    {
        GetView(i).Clear();
        SetModifiedMark(i, false);
        GetTabCtrl().UpdateLayout();
        GetTabCtrl().Invalidate();
    }
    UpdateStatusBar();
}

void CMainFrame::OnLogClear(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    ClearLog();
}

void CMainFrame::OnLogCrop(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    auto selection = GetView().GetSelectedRange();
    if (selection.count < 2)
    {
        return;
    }

    LogFile temp;
    temp.Append(m_logFile, selection.beginLine, selection.endLine);
    std::swap(temp, m_logFile);

    m_logSources.ResetTimer();
    int views = GetViewCount();
    for (int i = 0; i < views; ++i)
    {
        auto& view = GetView(i);
        view.Clear();
        view.ResetToLine(0);
        SetModifiedMark(i, false);
        GetTabCtrl().UpdateLayout();
        GetTabCtrl().Invalidate();
    }
    UpdateStatusBar();
}

void CMainFrame::OnLinkViews(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    m_linkViews = !m_linkViews;
}

void CMainFrame::OnAutoNewline(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    m_logSources.SetAutoNewLine(!m_logSources.GetAutoNewLine());
}

void CMainFrame::OnProcessPrefix(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    m_logSources.SetProcessPrefix(!m_logSources.GetProcessPrefix());
}

void CMainFrame::OnHide(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    m_hide = !m_hide;
}

bool CMainFrame::GetAlwaysOnTop() const
{
    return (GetWindowLong(GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
}

void CMainFrame::SetAlwaysOnTop(bool value)
{
    SetWindowPos(value ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void CMainFrame::OnAlwaysOnTop(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    SetAlwaysOnTop(!GetAlwaysOnTop());
}

bool CMainFrame::IsPaused() const
{
    return m_pLocalReader == nullptr;
}

void CMainFrame::Pause()
{
    SetTitle(L"Paused");
    if (m_pLocalReader != nullptr)
    {
        m_logSources.Remove(m_pLocalReader);
        m_pLocalReader = nullptr;
    }
    if (m_pGlobalReader != nullptr)
    {
        m_logSources.Remove(m_pGlobalReader);
        m_pGlobalReader = nullptr;
    }
    if (m_pKernelReader != nullptr)
    {
        m_logSources.Remove(m_pKernelReader);
        m_pKernelReader = nullptr;
    }
    m_logSources.AddMessage("<paused>");
}

void CMainFrame::UpdateTitle()
{
    std::wstring title = L"Paused";
    if ((m_pLocalReader != nullptr) && (m_pGlobalReader != nullptr))
    {
        title = L"Capture Win32 & Global Win32 Messages";
    }
    else if (m_pLocalReader != nullptr)
    {
        title = L"Capture Win32";
    }
    else if (m_pGlobalReader != nullptr)
    {
        title = L"Capture Global Win32";
    }
    SetTitle(title);
}

void CMainFrame::Resume()
{
    SetTitle();

    if (m_pLocalReader == nullptr)
    {
        try
        {
            m_pLocalReader = m_logSources.AddDBWinReader(false);
        }
        catch (std::exception&)
        {
            MessageBox(L"Unable to capture Win32 Messages.\n"
                       L"\n"
                       L"Another DebugView++ (or similar application) might be running.\n",
                m_applicationName.c_str(), MB_ICONERROR | MB_OK);
            return;
        }
    }

    if (m_tryGlobal && m_pGlobalReader == nullptr)
    {
        try
        {
            m_pGlobalReader = m_logSources.AddDBWinReader(true);
        }
        catch (std::exception&)
        {
            MessageBox(L"Unable to capture Global Win32 Messages.\n"
                       L"\n"
                       L"Make sure you have appropriate permissions.\n"
                       L"\n"
                       L"You may need to start this application by right-clicking it and selecting\n"
                       L"'Run As Administator' even if you have administrator rights.",
                m_applicationName.c_str(), MB_ICONERROR | MB_OK);
            m_tryGlobal = false;
        }
    }

    if (m_tryKernel && m_pKernelReader == nullptr)
    {
        try
        {
            m_pKernelReader = m_logSources.AddKernelReader();
        }
        catch (std::exception&)
        {
            MessageBox(L"Unable to capture Kernel Messages.\n"
                       L"\n"
                       L"Make sure you have appropriate permissions.\n"
                       L"\n"
                       L"You may need to start this application by right-clicking it and selecting\n"
                       L"'Run As Administator' even if you have administrator rights.",
                m_applicationName.c_str(), MB_ICONERROR | MB_OK);
            m_tryKernel = false;
        }
    }
    UpdateTitle();
}

void CMainFrame::OnLogPause(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    if (IsPaused())
    {
        Resume();
    }
    else
    {
        Pause();
    }
}

void CMainFrame::OnLogGlobal(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    m_tryGlobal = (m_pGlobalReader == nullptr);

    if (m_tryGlobal)
    {
        Resume();
    }
    else
    {
        m_logSources.Remove(m_pGlobalReader);
        m_pGlobalReader = nullptr;
    }
    UpdateTitle();
}

static const auto driver_name = "C:\\Windows\\System32\\drivers\\dbgvpp.sys";

void WriteDriverFromResource()
{
    if (std::filesystem::exists(driver_name))
    {
        return;
    }
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
                std::cout << "write to " << driver_name << "\n";
                std::ofstream outFile(driver_name, std::ios::binary);
                outFile.write(static_cast<const char*>(pLockedRes), dwSize);
                outFile.close();
            }
        }
    }
}

void RemoveDriver()
{
    std::filesystem::remove(driver_name);
}

void CMainFrame::OnLogKernel(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    m_tryKernel = (m_pKernelReader == nullptr);

    if (m_tryKernel)
    {
        WriteDriverFromResource();
        Resume();
    }
    else
    {
        m_logSources.Remove(m_pKernelReader);
        m_pKernelReader = nullptr;
        RemoveDriver();
    }
    UpdateTitle();
}

void CMainFrame::OnLogKernelVerbose(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    m_verboseKernelMessage = !m_verboseKernelMessage;
    if (m_pKernelReader)
    {
        m_pKernelReader->SetVerbose(m_verboseKernelMessage);
    }
}

void CMainFrame::OnLogKernelPassThrough(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    m_passthroughMode = !m_passthroughMode;
    if (m_pKernelReader)
    {
        m_pKernelReader->SetPassThrough(m_passthroughMode);
    }
}

void CMainFrame::OnLogHistory(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CHistoryDlg dlg(m_logFile.GetHistorySize(), m_logFile.GetHistorySize() == 0);
    if (dlg.DoModal() == IDOK)
    {
        m_logFile.SetHistorySize(dlg.GetHistorySize());
    }
}

std::wstring GetExecutionPath()
{
    auto path = std::filesystem::absolute(Win32::GetModuleFilename());
    return path.remove_filename().c_str();
}

void CMainFrame::OnLogDebugviewAgent(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    if (m_pDbgviewReader == nullptr)
    {
        std::string dbgview = stringbuilder() << Win32::GetExecutionPath() << "\\dbgview.exe";
        if (std::filesystem::exists(dbgview.c_str()))
        {
            std::string cmd = stringbuilder() << "start \"\" " << dbgview << " /a";
            system(cmd.c_str());
        }
        else
        {
            m_logSources.AddMessage("dbgview.exe not found");
        }
        m_pDbgviewReader = m_logSources.AddDbgviewReader("127.0.0.1");
    }
    else
    {
        m_logSources.Remove(m_pDbgviewReader);
        m_pDbgviewReader = nullptr;
    }
}

void CMainFrame::OnViewFilter(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CFilterDlg dlg(GetView().GetName(), GetView().GetFilters());
    if (dlg.DoModal() != IDOK)
    {
        return;
    }

    int tabIdx = GetTabCtrl().GetCurSel();
    GetTabCtrl().GetItem(tabIdx)->SetText(dlg.GetName().c_str());
    GetTabCtrl().UpdateLayout();
    GetTabCtrl().Invalidate();
    GetView().SetName(dlg.GetName());
    GetView().SetFilters(dlg.GetFilters());
    SaveSettings();
}

void CMainFrame::OnViewClose(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CloseView(GetTabCtrl().GetCurSel());
}

void CMainFrame::OnViewDuplicate(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    auto name = GetView().GetName() + L" (copy)";
    CFilterDlg dlg(name, GetView().GetFilters());
    if (dlg.DoModal() != IDOK)
    {
        return;
    }

    AddFilterView(dlg.GetName(), dlg.GetFilters());

    //auto &view = GetView();
    //auto newLogView = std::make_shared<CLogView>(view);        // no copy constructor available
    //newLogView->SetName(view.GetName() + L" (copy)");
    //AddFilterView(newLogView);

    SaveSettings();
}

void CMainFrame::OnSources(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CSourcesDlg dlg(m_sourceInfos);
    if (dlg.DoModal() != IDOK)
    {
        return;
    }

    auto sourceInfos = dlg.GetSourceInfos();
    UpdateLogSources(sourceInfos);

    m_sourceInfos = sourceInfos;
}

void CMainFrame::UpdateLogSources(const std::vector<SourceInfo>& sources)
{
    m_logSources.RemoveSources([this](LogSource* logsource) {
        if (logsource == m_pLocalReader)
        {
            return false;
        }
        if (logsource == m_pGlobalReader)
        {
            return false;
        }
        return true;
    });

    for (auto& sourceInfo : sources)
    {
        if (sourceInfo.enabled)
        {
            AddLogSource(sourceInfo);
        }
    }
}

void CMainFrame::AddLogSource(const SourceInfo& info)
{
    switch (info.type)
    {
    case SourceType::DebugViewAgent: m_logSources.AddDbgviewReader(Str(info.address)); break;
    case SourceType::Udp: m_logSources.AddUDPReader(info.port); break;
    case SourceType::Tcp: throw std::exception("SourceType::Tcp not implememted");
    default:
        // do nothing
        throw std::exception("SourceType not implememted");
    }
}

void CMainFrame::CloseView(int i)
{
    int views = GetViewCount();
    if (i >= 0 && i < views)
    {
        // DeleteItem actually calls delete on the contained item
        GetTabCtrl().DeleteItem(i, false);
        GetTabCtrl().SetCurSel(i == views - 1 ? i - 1 : i);
        if (GetViewCount() == 1)
        {
            HideTabControl();
        }
    }
}

void CMainFrame::OnViewFind(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    m_findDlg.SetFocus();
}

void CMainFrame::OnViewFont(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CFontDialog dlg(&m_logfont, CF_SCREENFONTS);
    if (dlg.DoModal(*this) == IDOK)
    {
        m_logfont = dlg.m_lf;
        SetLogFont();
    }
}

void CMainFrame::SetLogFont()
{
    Win32::HFont hFont(CreateFontIndirect(&m_logfont));
    if (!hFont)
    {
        return;
    }

    int views = GetViewCount();
    for (int i = 0; i < views; ++i)
    {
        GetView(i).SetFont(hFont.get());
    }
    m_hFont = std::move(hFont);
}

void CMainFrame::OnAppAbout(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
    CAboutDlg dlg;
    dlg.DoModal();
}

int CMainFrame::GetViewCount() const
{
    return m_TabCtrl.GetItemCount();
}

CLogView& CMainFrame::GetView(int i)
{
    assert(i >= 0 && i < GetViewCount());
    return GetTabCtrl().GetItem(i)->GetView();
}

CLogView& CMainFrame::GetView()
{
    return GetView(std::max(0, GetTabCtrl().GetCurSel()));
}

bool IsClearBufferMessage(const std::string& message)
{
    return message.find("DBGVIEWCLEAR") == 0;
}

void CMainFrame::AddMessage(const Message& message)
{
    if (IsClearBufferMessage(message.text))
    {
        ClearLog();
        return;
    }

    int beginIndex = m_logFile.BeginIndex();
    int index = m_logFile.EndIndex();
    m_logFile.Add(message);
    int views = GetViewCount();
    for (int i = 0; i < views; ++i)
    {
        GetView(i).Add(beginIndex, index, message);
    }
}

} // namespace debugviewpp
} // namespace fusion
