// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include "stdafx.h"
#include <boost/algorithm/string.hpp>
#include <atlstr.h>
#include "Win32Lib/utilities.h"
#include "DebugView++Lib/LogFilter.h"
#include "Resource.h"
#include "FilterDlg.h"

namespace fusion {
namespace debugviewpp {

static COLORREF HighlightColors[16] = 
{
	RGB(255, 255, 255), // white
	RGB(192, 192, 192), // light-grey
	RGB(128, 128, 128), // mid-grey
	RGB( 64,  64,  64), // dark-grey
	RGB(  0,   0,   0), // black
	RGB( 27, 161, 226), // blue
	RGB(160,  80,   0), // brown
	RGB( 51, 153,  51), // green
	RGB(162, 193,  57), // lime
	RGB(216,   0, 115), // magenta
	RGB(240, 150,   9), // mango (orange)
	RGB(230, 113, 184), // pink
	RGB(162,   0, 255), // purple
	RGB(229,  20,   0), // red
	RGB(  0, 171, 169), // teal (viridian)
	RGB(255, 255, 255), // white
};

void InitializeCustomColors()
{
	auto colors = ColorDialog::GetCustomColors();
	for (int i = 0; i < 16; ++i)
		colors[i] = HighlightColors[i];
}

bool CustomColorsInitialized = (InitializeCustomColors(), true);

BEGIN_MSG_MAP_TRY(CFilterDlg)
	MSG_WM_INITDIALOG(OnInitDialog)
	MSG_WM_DESTROY(OnDestroy)
	MSG_WM_SIZE(OnSize)
	COMMAND_ID_HANDLER_EX(IDC_FILTER_SAVE, OnSave)
	COMMAND_ID_HANDLER_EX(IDC_FILTER_LOAD, OnLoad)
	COMMAND_ID_HANDLER_EX(IDC_FILTER_REGEX, OnRegEx)
	COMMAND_ID_HANDLER_EX(IDC_FILTER_REMOVEALL, OnClearAll)
	COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
	COMMAND_ID_HANDLER_EX(IDOK, OnOk)
	NOTIFY_CODE_HANDLER_EX(TCN_SELCHANGE, OnTabSelChange)
	REFLECT_NOTIFICATIONS()
	CHAIN_MSG_MAP(CDialogResize<CFilterDlg>)
END_MSG_MAP_CATCH(ExceptionHandler)

static const FilterType::type MessageFilterTypes[] =
{
	FilterType::Include,
	FilterType::Exclude,
	FilterType::Highlight,
	FilterType::Token,
	FilterType::Stop,
	FilterType::Track,
	FilterType::Once,
	FilterType::Clear,
	FilterType::Beep,
	FilterType::MatchColor
};

static const FilterType::type ProcessFilterTypes[] =
{
	FilterType::Include,
	FilterType::Exclude,
	FilterType::Highlight,
	FilterType::Stop,
	FilterType::Track,
	FilterType::Once,
	FilterType::Beep
};

static const MatchType::type MatchTypes[] =
{
	MatchType::Simple,
	MatchType::Wildcard,
	MatchType::Regex,
	MatchType::RegexGroups
};

CFilterDlg::CFilterDlg(const std::wstring& name, const LogFilter& filters) :
	m_messagePage(MessageFilterTypes, MatchTypes),
	m_processPage(ProcessFilterTypes, MatchTypes),
	m_name(name),
	m_filter(filters)
{
	m_messagePage.SetFilters(filters.messageFilters);
	m_processPage.SetFilters(filters.processFilters);
}

std::wstring CFilterDlg::GetName() const
{
	return m_name;
}

LogFilter CFilterDlg::GetFilters() const
{
	return m_filter;
}

void CFilterDlg::ExceptionHandler()
{
	MessageBox(WStr(GetExceptionMessage()).c_str(), LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
}

BOOL CFilterDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
	SetDlgItemText(IDC_NAME, m_name.c_str());

	m_tabCtrl.Attach(GetDlgItem(IDC_TAB));
	m_tabCtrl.AddItem(L"Messages");
	m_tabCtrl.AddItem(L"Processes");
	CRect tabRect;
	m_tabCtrl.GetWindowRect(&tabRect);
	m_tabCtrl.AdjustRect(false, &tabRect);
	m_tabCtrl.ScreenToClient(&tabRect);

	CRect dlgRect;
	GetClientRect(&dlgRect);
	m_border.cx = dlgRect.Width() - tabRect.Width();
	m_border.cy = dlgRect.Height() - tabRect.Height();

	m_messagePage.Create(m_tabCtrl, tabRect);
	m_messagePage.MoveWindow(&tabRect);
	m_messagePage.ShowWindow(SW_SHOW);

	m_processPage.Create(m_tabCtrl, tabRect);
	m_processPage.MoveWindow(&tabRect);
	m_processPage.ShowWindow(SW_HIDE);

	m_regExDlg.Create(*this, 0);
	m_regExDlg.ShowWindow(SW_HIDE);

	CenterWindow(GetParent());
	DlgResize_Init();

	return TRUE;
}

void CFilterDlg::OnDestroy()
{
}

void CFilterDlg::OnSize(UINT /*nType*/, CSize size)
{
	RECT rect;
	m_tabCtrl.GetWindowRect(&rect);
	m_tabCtrl.AdjustRect(false, &rect);
	m_tabCtrl.ScreenToClient(&rect);
	rect.right = rect.left + size.cx - m_border.cx;
	rect.bottom = rect.top + size.cy - m_border.cy;

	m_messagePage.MoveWindow(&rect);
	m_processPage.MoveWindow(&rect);
	SetMsgHandled(FALSE);
}

LRESULT CFilterDlg::OnTabSelChange(NMHDR* /*pnmh*/)
{
	SelectTab(m_tabCtrl.GetCurSel());
	return 0;
}

void CFilterDlg::SelectTab(int tab)
{
	m_tabCtrl.SetCurSel(tab);
	m_messagePage.ShowWindow(tab == 0 ? SW_SHOW : SW_HIDE);
	m_processPage.ShowWindow(tab == 1 ? SW_SHOW : SW_HIDE);
}

std::wstring GetFileNameExt(const std::wstring& fileName)
{
	auto it = fileName.find_last_of('.');
	if (it == fileName.npos)
		return std::wstring();
	return fileName.substr(it + 1);
}

void CFilterDlg::OnSave(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	CFileDialog dlg(false, L".xml", m_name.c_str(), OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
		L"XML Files (*.xml)\0*.xml\0"
		L"JSON Files (*.json)\0*.json\0"
		L"All Files\0*.*\0"
		L"\0", 0);
	dlg.m_ofn.nFilterIndex = 0;
	dlg.m_ofn.lpstrTitle = L"Save DebugView Filter";
	if (dlg.DoModal() != IDOK)
		return;

	LogFilter filter;
	auto name = fusion::GetDlgItemText(*this, IDC_NAME);
	filter.messageFilters = m_messagePage.GetFilters();
	filter.processFilters = m_processPage.GetFilters();

	auto ext = GetFileNameExt(dlg.m_szFileName);
	auto fileName = Str(dlg.m_szFileName).str();
	if (boost::iequals(ext, L"json"))
		SaveJson(fileName, Str(name), filter);
	else /* if (boost::iequals(ext, L"xml")) */
		SaveXml(fileName, Str(name), filter);
}

void CFilterDlg::OnLoad(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	CFileDialog dlg(true, L".xml", m_name.c_str(), OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		L"XML Files (*.xml)\0*.xml\0"
		L"JSON Files (*.json)\0*.json\0"
		L"All Files\0*.*\0"
		L"\0", 0);
	dlg.m_ofn.nFilterIndex = 0;
	dlg.m_ofn.lpstrTitle = L"Load DebugView Filter";

	// notice subtle behaviour, see http://msdn.microsoft.com/en-us/library/ms646839 at lpstrInitialDir 
	std::wstring path;
	wchar_t szPath[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_PERSONAL, nullptr, 0, szPath)))
	{
		path = szPath;
		path += L"\\DebugView++ Filters";
		dlg.m_ofn.lpstrInitialDir = path.c_str();
	}

	if (dlg.DoModal() != IDOK)
		return;

	auto ext = GetFileNameExt(dlg.m_szFileName);
	auto fileName = Str(dlg.m_szFileName).str();
	FilterData data;
	if (boost::iequals(ext, L"json"))
		data = LoadJson(fileName);
	else /* if (boost::iequals(ext, L"xml")) */
		data = LoadXml(fileName);

	SetDlgItemTextA(*this, IDC_NAME, data.name.c_str());

	auto msgFilters = m_messagePage.GetFilters();
	msgFilters.insert(msgFilters.end(), data.filter.messageFilters.begin(), data.filter.messageFilters.end());
	m_messagePage.SetFilters(msgFilters);

	auto procFilters = m_processPage.GetFilters();
	procFilters.insert(procFilters.end(), data.filter.processFilters.begin(), data.filter.processFilters.end());
	m_processPage.SetFilters(procFilters);
}

void CFilterDlg::OnRegEx(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	m_regExDlg.ShowWindow(SW_SHOW);
}

void CFilterDlg::OnClearAll(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	m_messagePage.SetFilters(std::vector<Filter>());
	m_processPage.SetFilters(std::vector<Filter>());
}

void CFilterDlg::OnCancel(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
	EndDialog(nID);
}

std::wstring GetRegexErrorDescription(std::regex_constants::error_type err)
{
	switch (err)
	{
	case std::regex_constants::error_badbrace: return L"Invalid count in { } expression";
	case std::regex_constants::error_badrepeat: return L"A repeat expression (one of '*', '?', '+', '{') was not preceded by an expression";
	case std::regex_constants::error_brace: return L"Unmatched '{' or '}'";
	case std::regex_constants::error_brack: return L"Unmatched '[' or ']'";
	case std::regex_constants::error_collate: return L"Invalid collating element name";
	case std::regex_constants::error_complexity: return L"Too complex";
	case std::regex_constants::error_ctype: return L"Invalid character class name";
	case std::regex_constants::error_escape: return L"Invalid escape sequence";
	case std::regex_constants::error_paren: return L"Unmatched '(' or ')'";
	case std::regex_constants::error_range: return L"Invalid character range specifier";
	case std::regex_constants::error_space: return L"Not enough resources available";
	case std::regex_constants::error_stack: return L"Not enough memory available";
	case std::regex_constants::error_backref: return L"Invalid back reference";
	default: return L"";
	}
}

void CFilterDlg::OnOk(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/)
{
	m_name = fusion::GetDlgItemText(*this, IDC_NAME);
	CFilterPage* pPage = nullptr;
	try
	{
		pPage = &m_messagePage;
		m_filter.messageFilters = m_messagePage.GetFilters();
		pPage = &m_processPage;
		m_filter.processFilters = m_processPage.GetFilters();
	}
	catch (std::regex_error& ex)
	{
		SelectTab(pPage == &m_processPage);
		MessageBox(WStr(L"Regular expression syntax error: " + GetRegexErrorDescription(ex.code())), LoadString(IDR_APPNAME).c_str(), MB_ICONERROR | MB_OK);
		pPage->ShowError();
		return;
	}
	EndDialog(nID);
}

} // namespace debugviewpp 
} // namespace fusion
