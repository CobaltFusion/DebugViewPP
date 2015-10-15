// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <vector>

#include "Resource.h"
#include "DebugView++Lib/LogSource.h"

namespace fusion {
namespace debugviewpp {

class CSourceDlg :
	public CDialogImpl<CSourceDlg>
{
public:
	CSourceDlg(const std::wstring& name, SourceType::type sourceType, const std::wstring& address, int port);
		
	enum { IDD = IDD_SOURCE };
	
	BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID);
	std::wstring GetName() const;
	SourceType::type GetSourceType() const;
	std::wstring GetAddress() const;
	int GetPort() const;

private:
	void ExceptionHandler();
	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnOk(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnTypeSelChange(UINT uNotifyCode, int nID, CWindow wndCtl);
	void UpdateUI();

	std::wstring m_name;
	SourceType::type m_sourceType;
	std::wstring m_address;
	int m_port;
};

} // namespace debugviewpp 
} // namespace fusion
