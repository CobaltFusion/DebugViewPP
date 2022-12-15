// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#pragma once

#include "CobaltFusion/AtlWinExt.h"
#include "resource.h"

#include "atleverything.h"

namespace fusion {
namespace debugviewpp {

class CRegExDlg : public CDialogImpl<CRegExDlg>,
                  public ExceptionHandler<CRegExDlg, std::exception>
{
public:
    enum
    {
        IDD = IDD_REGEX
    };

private:
    DECLARE_MSG_MAP()

    void OnException() const;
    void OnException(const std::exception& ex) const;

    BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/);
    void OnClose(UINT /*uNotifyCode*/, int nID, CWindow /*wndCtl*/);
};

} // namespace debugviewpp
} // namespace fusion
