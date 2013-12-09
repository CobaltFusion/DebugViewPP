// (C) Copyright Gert-Jan de Vos 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "PropertyItem.h"
#include "PropertyItemEditors.h"
#include "PropertyItemImpl.h"

class CPropertyColorItem : public CPropertyItem
{
public:
	explicit CPropertyColorItem(COLORREF color) :
		CPropertyItem(L"", 0),
		m_dlg(color, CC_ANYCOLOR)
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
			m_dlg.DoModal(m_hWndOwner);
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
	CColorDialog m_dlg;
};

inline HPROPERTY PropCreateColorItem(COLORREF color)
{
	return new CPropertyColorItem(color);
}
