// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include <vector>
#include "CobaltFusion/AtlWinExt.h"
#include "DebugView++Lib/LogSource.h"
#include "resource.h"

namespace fusion {
namespace debugviewpp {

class CSourceDlg : public CDialogImpl<CSourceDlg>,
                   public ExceptionHandler<CSourceDlg, std::exception>
{
public:
    CSourceDlg(std::wstring name, SourceType::type sourceType, std::wstring address, int port);

    enum
    {
        IDD = IDD_SOURCE
    };

    std::wstring GetName() const;
    SourceType::type GetSourceType() const;
    std::wstring GetAddress() const;
    int GetPort() const;

private:
    DECLARE_MSG_MAP()

    void OnException();
    void OnException(const std::exception& ex);
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
