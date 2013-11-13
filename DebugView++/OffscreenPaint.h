//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)


#pragma once

#include <atlgdi.h>

namespace gj {

template <typename T>
class COffscreenPaint
{
public:
	BEGIN_MSG_MAP(COffscreenPaint)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_PRINTCLIENT, OnPaint)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
	END_MSG_MAP()

	LRESULT OnPaint(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		T* pT = static_cast<T*>(this);
		if (wParam)
		{
			CDCHandle hdc(reinterpret_cast<HDC>(wParam));
			RECT rc;
			hdc.GetClipBox(&rc);
			CMemoryDC memdc(hdc, rc);
			pT->DoPaint(memdc.m_hDC, memdc.m_rcPaint);
		}
		else
		{
			CPaintDC dc(pT->m_hWnd);
			CMemoryDC memdc(dc.m_hDC, dc.m_ps.rcPaint);
			pT->DoPaint(memdc.m_hDC, dc.m_ps.rcPaint);
		}
		return 0;
	}

	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		return 1; // handled; no need to erase background; do it in DoPaint();
	}

	void DoPaint(CDCHandle dc, const RECT& rcClip)
	{
		ATLASSERT(false); // must override this
	}
};

} // namespace gj
