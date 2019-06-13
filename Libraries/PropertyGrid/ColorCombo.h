#if !defined(AFX_COLORCOMBO_H__20020314_DDA3_617F_0D88_0080AD509054__INCLUDED_)
#define AFX_COLORCOMBO_H__20020314_DDA3_617F_0D88_0080AD509054__INCLUDED_

#pragma once

/////////////////////////////////////////////////////////////////////////////
// ColorComboPicker - Simple color picker controls
//
// Written by Bjarke Viksoe (bjarke@viksoe.dk)
// Copyright (c) 2002-2003 Bjarke Viksoe.
//
// Add the following macro to the parent's message map:
//   REFLECT_NOTIFICATIONS()
//
// This code may be used in compiled form in any way you desire. This
// source file may be redistributed by any means PROVIDING it is
// not sold for profit without the authors written consent, and
// providing that this notice and the authors name is included.
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability if it causes any damage to you or your
// computer whatsoever. It's free, so don't hassle me about it.
//
// Beware of bugs.
//

#ifndef __cplusplus
#error WTL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLCTRLS_H__
#error ColorCombo.h requires atlctrls.h to be included first
#endif


/////////////////////////////////////////////////////////////////////////////
//

template <class T, class TBase = CComboBox, class TWinTraits = CControlWinTraits>
class ATL_NO_VTABLE CColorPickerImpl : public CWindowImpl<T, TBase, TWinTraits>,
									   public COwnerDraw<T>
{
public:
	DECLARE_WND_SUPERCLASS(NULL, TBase::GetWndClassName())

	enum
	{
		s_cxIndent = 1,
		s_cyIndent = 1
	};

	// Operations

	BOOL SubclassWindow(HWND hWnd)
	{
		ATLASSERT(m_hWnd == NULL);
		ATLASSERT(::IsWindow(hWnd));
		BOOL bRet = CWindowImpl<T, TBase, TWinTraits>::SubclassWindow(hWnd);
		if (bRet)
			_Init();
		return bRet;
	}

	int AddColor(int iIndex, COLORREF clr)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		int iItem = InsertString(iIndex, _T(""));
		if (iItem == -1)
			return -1;
		SetItemData(iItem, (DWORD)clr);
		return iItem;
	}

	BOOL RemoveColor(COLORREF clr)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		int iItem = FindColor(clr);
		if (iItem == -1)
			return FALSE;
		return DeleteString(iItem);
	}

	BOOL ChangeColor(int iIndex, COLORREF clr)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		int iItem = FindColor(clr);
		if (iItem == -1)
			return FALSE;
		SetItemData(iItem, (DWORD)clr);
		Invalidate();
		return TRUE;
	}

	BOOL SelectColor(COLORREF clr)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		int iItem = FindColor(clr);
		if (iItem == -1)
			return FALSE;
		return SetCurSel(iItem);
	}

	COLORREF GetSelectedColor() const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		int iItem = GetCurSel();
		if (iItem == -1)
			return (COLORREF)-1; // Returns -1 if none if selected
		return (COLORREF)GetItemData(iItem) & 0xFFFFFF;
	}

	int FindColor(COLORREF clr) const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		int nCount = GetCount();
		for (int i = 0; i < nCount; i++)
		{
			if ((GetItemData(i) & 0xFFFFFF) == (DWORD)clr)
				return i;
		}
		return -1;
	}

	// Implementation

	void _Init()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		// Need to set these in resource editor
		// If it's a ListBox, use LBS_OWNERDRAWFIXED.
		ATLASSERT(GetStyle() & CBS_OWNERDRAWFIXED);
	}

	// Message map and handlers

	BEGIN_MSG_MAP(CColorPickerImpl)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		CHAIN_MSG_MAP_ALT(COwnerDraw<T>, 1)
	END_MSG_MAP()

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		LRESULT lRes = DefWindowProc();
		_Init();
		return lRes;
	}

	// COwnerDraw

	void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
	{
		if (lpDrawItemStruct->itemID == -1)
			return;

		CDCHandle dc = lpDrawItemStruct->hDC;
		RECT rc = lpDrawItemStruct->rcItem;
		COLORREF clr = (COLORREF)lpDrawItemStruct->itemData;
		bool bSelected = lpDrawItemStruct->itemState & ODS_SELECTED;

		// Never paint selected color for combobox itself (edit part)
		if (lpDrawItemStruct->itemState & ODS_COMBOBOXEDIT)
			bSelected = false;

		// Fill background and color item
		CBrushHandle brBack = ::GetSysColorBrush(bSelected ? COLOR_HIGHLIGHT : COLOR_WINDOW);
		dc.FillRect(&rc, brBack);
		::InflateRect(&rc, -s_cxIndent, -s_cyIndent);
		dc.FillSolidRect(&rc, RGB(0, 0, 0)); // Hmm, should have been a call to ::Rectangle()
		::InflateRect(&rc, -1, -1);
		dc.FillSolidRect(&rc, clr & 0xFFFFFF);

#if defined(IDS_CUSTOM) && (_ATL_VER < 0x0700)
		// NOTE: A little hack to display the "Custom" text on
		//       a color item. Simply add 0xFF000000 to the value of
		//       the color and remember to include the "resource.h" header
		//       before this file...
		if ((clr & 0xFF000000) == 0xFF000000)
		{
			TCHAR szCustom[32] = {0};
			::LoadString(_Module.GetResourceInstance(), IDS_CUSTOM, szCustom, 31);
			dc.SetBkMode(TRANSPARENT);
			dc.SetTextColor(RGB(255, 255, 255));
			dc.DrawText(szCustom, -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		}
#endif // IDS_CUSTOM
	}
};

class CColorPickerComboCtrl : public CColorPickerImpl<CColorPickerComboCtrl, CComboBox, CWinTraitsOR<CBS_OWNERDRAWFIXED | CBS_DROPDOWNLIST | CBS_HASSTRINGS>>
{
public:
	DECLARE_WND_SUPERCLASS(_T("WTL_ColorPickerComboBox"), GetWndClassName())
};

class CColorPickerListCtrl : public CColorPickerImpl<CColorPickerListCtrl, CListBox, CWinTraitsOR<LBS_OWNERDRAWFIXED | LBS_HASSTRINGS>>
{
public:
	DECLARE_WND_SUPERCLASS(_T("WTL_ColorPickerListBox"), GetWndClassName())
};


#endif // !defined(AFX_COLORCOMBO_H__20020314_DDA3_617F_0D88_0080AD509054__INCLUDED_)
