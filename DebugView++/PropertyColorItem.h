// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "PropertyItem.h"
#include "PropertyItemEditors.h"
#include "PropertyItemImpl.h"
#include "DebugView++Lib/Colors.h"
#include "Win32/Win32Lib.h"

namespace fusion {
namespace debugviewpp {

class ColorDialog : public CColorDialogImpl<ColorDialog>
{
public:
	ColorDialog(const wchar_t* title, COLORREF clrInit = 0, DWORD dwFlags = 0, HWND hWndParent = nullptr) :
		CColorDialogImpl<ColorDialog>(clrInit == Colors::Auto ? RGB(255, 255, 255) : clrInit, dwFlags, hWndParent),
		m_title(title),
		m_auto(clrInit == Colors::Auto),
		m_showAuto(false)
	{
	}

	BEGIN_MSG_MAP_EX(ColorDialog)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_DESTROY(OnDestroy)
		CHAIN_MSG_MAP(CColorDialogImpl<ColorDialog>)
	END_MSG_MAP()

	BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
	{
		SetWindowText(m_title.c_str());
		if (m_showAuto)
		{
			RECT rect = { 166, 160, 216, 176 };
			m_btnAuto.Create(*this, &rect, L"Auto", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_TEXT | BS_LEFT, 0);
			m_btnAuto.SetFont(GetFont(), FALSE);
			m_btnAuto.SetCheck(m_auto ? BST_CHECKED : BST_UNCHECKED);
		}
		return TRUE;
	}

	void OnDestroy()
	{
		if (m_btnAuto.IsWindow())
		{
			m_auto = m_showAuto && m_btnAuto.GetCheck() == BST_CHECKED;
			m_btnAuto.DestroyWindow();
		}
		else
		{
			m_auto = false;
		}
	}

	void ShowAuto(bool show)
	{
		m_showAuto = show;
	}

	void SetCurrentColor(COLORREF color)
	{
		m_auto = color == Colors::Auto;
		CColorDialogImpl<ColorDialog>::SetCurrentColor(m_auto ? RGB(255, 255, 255) : color);
	}

	COLORREF GetColor() const
	{
		return m_auto ? Colors::Auto : CColorDialogImpl<ColorDialog>::GetColor();
	}

private:
	std::wstring m_title;
	CButton m_btnAuto;
	bool m_auto;
	bool m_showAuto;
};

class CPropertyColorItem : public CPropertyItem
{
public:
	CPropertyColorItem(const wchar_t* name, COLORREF color) :
		CPropertyItem(name, 0),
		m_dlg(name, color, CC_ANYCOLOR)
	{
	}

	COLORREF GetColor() const
	{
		return m_dlg.GetColor();
	}

	void SetColor(COLORREF color)
	{
		m_dlg.SetCurrentColor(color);
	}

	void ShowAuto(bool show)
	{
		m_dlg.ShowAuto(show);
	}

    virtual BOOL Activate(UINT action, LPARAM /*lParam*/) override
	{
		if (!IsEnabled())
			return FALSE;

		switch (action)
		{
		case PACT_SPACE:
		case PACT_CLICK:
		case PACT_DBLCLICK:
			if (m_dlg.DoModal(m_hWndOwner) == IDOK)
			{
				// Let control owner know
				NMPROPERTYITEM nmh = { m_hWndOwner, ::GetDlgCtrlID(m_hWndOwner), PIN_ITEMCHANGED, this };
				::SendMessage(::GetParent(m_hWndOwner), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh);
			}
			break;
		}
		return TRUE;
	}

    virtual void DrawValue(PROPERTYDRAWINFO& di) override
	{
		CDCHandle dc(di.hDC);
		RECT rect = di.rcItem;
		if (!IsEnabled())
			return dc.FillSolidRect(&rect, di.clrDisabledBack);

		dc.FillSolidRect(&rect, di.clrBorder);
		::InflateRect(&rect, -1, -1);
		dc.FillSolidRect(&rect, RGB(0, 0, 0));
		::InflateRect(&rect, -1, -1);
		auto color = GetColor();
		if (color == Colors::Auto)
		{
			Win32::ScopedBkColor bg(dc, RGB(255, 255, 255));
			Win32::ScopedTextColor fg(dc, RGB(0, 0, 0));
			Win32::ScopedTextAlign ta(dc, TA_CENTER | TA_BOTTOM);
			dc.ExtTextOut((rect.left + rect.right)/2, rect.bottom, ETO_OPAQUE, &rect, L"A", 1);
		}
		else
		{
			dc.FillSolidRect(&rect, color & 0xFFFFFF);
		}
	}

    virtual BOOL GetValue(VARIANT* pValue) const override
	{
		CComVariant var(GetColor());
		return SUCCEEDED(var.Detach(pValue));
	}

    virtual BOOL SetValue(const VARIANT& value) override
	{
		CComVariant var;
		if (FAILED(VariantChangeType(&var, &value, 0, VT_COLOR)))
			return FALSE;
		SetColor(var.intVal);
		return TRUE;
	}

private:
	ColorDialog m_dlg;
};

inline CPropertyColorItem* PropCreateColorItem(const wchar_t* name, COLORREF color)
{
	return new CPropertyColorItem(name, color);
}

} // namespace debugviewpp
} // namespace fusion
