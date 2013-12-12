// (C) Copyright Gert-Jan de Vos 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "PropertyItem.h"
#include "PropertyItemEditors.h"
#include "PropertyItemImpl.h"

class ColorDialog : public CColorDialogImpl<ColorDialog>
{
public:
	ColorDialog(const wchar_t* title, COLORREF clrInit = 0, DWORD dwFlags = 0, HWND hWndParent = nullptr) :
		CColorDialogImpl<ColorDialog>(clrInit, dwFlags, hWndParent),
		m_title(title)
	{
	}

	BEGIN_MSG_MAP(ColorDialog)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		SetWindowText(m_title.c_str());
		return 0;
	}

private:
	std::wstring m_title;
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

	BOOL Activate(UINT action, LPARAM /*lParam*/)
	{
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
		dc.FillSolidRect(&rect, di.clrBorder);
		::InflateRect(&rect, -1, -1);
		dc.FillSolidRect(&rect, RGB(0, 0, 0));
		::InflateRect(&rect, -1, -1);
		dc.FillSolidRect(&rect, GetColor() & 0xFFFFFF);
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
	ColorDialog m_dlg;
};

inline CPropertyColorItem* PropCreateColorItem(const wchar_t* name, COLORREF color)
{
	return new CPropertyColorItem(name, color);
}
