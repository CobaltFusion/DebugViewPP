// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "PropertyItem.h"
#include "PropertyItemEditors.h"
#include "PropertyItemImpl.h"
#include "Win32Lib/Win32Lib.h"

namespace Colors
{
	const COLORREF Auto = 0x80808080;
}

class ColorDialog : public CColorDialogImpl<ColorDialog>
{
public:
	ColorDialog(const wchar_t* title, COLORREF clrInit = 0, DWORD dwFlags = 0, HWND hWndParent = nullptr) :
		CColorDialogImpl<ColorDialog>(clrInit, dwFlags, hWndParent),
		m_title(title)
	{
	}

	BEGIN_MSG_MAP(ColorDialog)
		MSG_WM_INITDIALOG(OnInitDialog)
		CHAIN_MSG_MAP(CColorDialogImpl<ColorDialog>)
	END_MSG_MAP()

	BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
	{
		SetWindowText(m_title.c_str());
		return TRUE;
	}

private:
	std::wstring m_title;
};

class ColorAutoDialog : public CColorDialogImpl<ColorAutoDialog>
{
public:
	ColorAutoDialog(const wchar_t* title, COLORREF clrInit = 0, DWORD dwFlags = 0, HWND hWndParent = nullptr) :
		CColorDialogImpl<ColorAutoDialog>(clrInit, dwFlags, hWndParent),
		m_title(title),
		m_auto(clrInit == Colors::Auto),
		m_showAuto(false)
	{
	}

	BEGIN_MSG_MAP_EX(ColorAutoDialog)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_DESTROY(OnDestroy)
		CHAIN_MSG_MAP(CColorDialogImpl<ColorAutoDialog>)
	END_MSG_MAP()

	BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
	{
		SetWindowText(m_title.c_str());
		if (m_showAuto)
		{
			RECT rect = { 164, 160, 220, 176 };
			m_btnAuto.Create(*this, &rect, L"Auto", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_TEXT | BS_LEFT, 0, IDD_AUTO);
			m_btnAuto.SetFont(GetFont(), FALSE);
			m_btnAuto.CheckDlgButton(IDD_AUTO, m_auto ? BST_CHECKED : BST_UNCHECKED);
		}
		return TRUE;
	}

	void OnDestroy()
	{
		m_auto = m_showAuto && m_btnAuto.GetCheck() == BST_CHECKED;
		if (m_btnAuto.IsWindow())
			m_btnAuto.DestroyWindow();
	}

	void HideAuto()
	{
		m_showAuto = false;
	}

	void ShowAuto()
	{
		m_showAuto = true;
	}

	void SetCurrentColor(COLORREF color)
	{
		CColorDialogImpl<ColorAutoDialog>::SetCurrentColor(color);
		m_auto = color == Colors::Auto;
	}

	COLORREF GetColor() const
	{
		return m_auto ? Colors::Auto : CColorDialogImpl<ColorAutoDialog>::GetColor();
	}

private:
	enum { IDD_AUTO = 1234 };
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

	void ShowAuto()
	{
		m_dlg.ShowAuto();
	}

	void HideAuto()
	{
		m_dlg.HideAuto();
	}

	BOOL Activate(UINT action, LPARAM /*lParam*/)
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

	void DrawValue(PROPERTYDRAWINFO& di)
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
			fusion::ScopedBkColor bg(dc, RGB(255, 255, 255));
			fusion::ScopedTextColor fg(dc, RGB(0, 0, 0));
			fusion::ScopedTextAlign ta(dc, TA_CENTER | TA_BOTTOM);
			dc.ExtTextOut((rect.left + rect.right)/2, rect.bottom, ETO_OPAQUE, &rect, L"A", 1);
		}
		else
		{
			dc.FillSolidRect(&rect, color & 0xFFFFFF);
		}
	}

	BOOL GetValue(VARIANT* pValue) const
	{
		CComVariant var(GetColor());
		return SUCCEEDED(var.Detach(pValue));
	}

	BOOL SetValue(const VARIANT& value)
	{
		CComVariant var;
		if (FAILED(VariantChangeType(&var, &value, 0, VT_COLOR)))
			return FALSE;
		SetColor(var.intVal);
		return TRUE;
	}

private:
	ColorAutoDialog m_dlg;
};

inline CPropertyColorItem* PropCreateColorItem(const wchar_t* name, COLORREF color)
{
	return new CPropertyColorItem(name, color);
}
