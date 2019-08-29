// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "CobaltFusion/AtlWinExt.h"
#include "Resource.h"

namespace fusion {
namespace debugviewpp {

class CRenameProcessDlg : public CDialogImpl<CRenameProcessDlg>,
                          public ExceptionHandler<CRenameProcessDlg, std::exception>
{
public:
    explicit CRenameProcessDlg(const std::wstring& name);

    enum
    {
        IDD = IDD_RENAMEPROCESS
    };

    std::wstring GetName() const;

private:
    DECLARE_MSG_MAP()

    void OnException();
    void OnException(const std::exception& ex);
    BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
    void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnOk(UINT uNotifyCode, int nID, CWindow wndCtl);
    void OnTypeSelChange(UINT uNotifyCode, int nID, CWindow wndCtl);

    std::wstring m_name;
};

} // namespace debugviewpp
} // namespace fusion
