#ifndef __PROPERTYITEM__H
#define __PROPERTYITEM__H

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CPropertyItem - Base Property implementation for the Property controls
//
// Written by Bjarke Viksoe (bjarke@viksoe.dk)
// Copyright (c) 2001-2003 Bjarke Viksoe.
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


/////////////////////////////////////////////////////////////////////////////
// Defines

// Control notifications (uses NMPROPERTYITEM structure)
#define PIN_FIRST               (0U-3000U)
#define PIN_LAST                (0U-3200U)

#define PIN_SELCHANGED          (PIN_FIRST-1)
#define PIN_ITEMCHANGING        (PIN_FIRST-2)
#define PIN_ITEMCHANGED         (PIN_FIRST-3)
#define PIN_COLLAPSING          (PIN_FIRST-4)
#define PIN_EXPANDING           (PIN_FIRST-5)
#define PIN_BROWSE              (PIN_FIRST-6)
#define PIN_CLICK               (PIN_FIRST-7)
#define PIN_DBLCLICK            (PIN_FIRST-8)
#define PIN_ADDITEM             (PIN_FIRST-9)

// Identifiers returned by GetKind()
#define PROPKIND_CATEGORY   0x0001
#define PROPKIND_SIMPLE     0x0002
#define PROPKIND_EDIT       0x0003
#define PROPKIND_LIST       0x0004
#define PROPKIND_BOOL       0x0005
#define PROPKIND_CHECK      0x0006
#define PROPKIND_SPIN       0x0007
#define PROPKIND_CONTROL    0x0008

// Activate actions
#define PACT_ACTIVATE       0x0001
#define PACT_EXPAND         0x0002
#define PACT_CLICK          0x0003
#define PACT_DBLCLICK       0x0004
#define PACT_BROWSE         0x0005
#define PACT_TAB            0x0006
#define PACT_SPACE          0x0007


// Draw structure
typedef struct
{
   HDC hDC;
   RECT rcItem;
   UINT state;
   //
   HFONT TextFont;
   HFONT CategoryFont;
   HPEN Border;
   COLORREF clrText;
   COLORREF clrBack;
   COLORREF clrSelText;
   COLORREF clrSelBack;
   COLORREF clrBorder;
   COLORREF clrDisabled;
   COLORREF clrDisabledBack;
   //
   TEXTMETRIC tmText;
   DWORD dwExtStyle;
} PROPERTYDRAWINFO;

// Custom control messages
#ifndef WM_USER_PROP_UPDATEPROPERTY
#define WM_USER_PROP_UPDATEPROPERTY   WM_USER+430
#define WM_USER_PROP_CANCELPROPERTY   WM_USER+431
#define WM_USER_PROP_CHANGEDPROPERTY  WM_USER+432
#define WM_USER_PROP_EXPAND           WM_USER+433
#define WM_USER_PROP_COLLAPSE         WM_USER+434
#define WM_USER_PROP_NAVIGATE         WM_USER+435
#define WM_USER_PROP_SETCHECKSTATE    WM_USER+436
#endif // WM_USER_xxx


/////////////////////////////////////////////////////////////////////////////
// Property class interface

class IProperty
{
public:
   virtual ~IProperty() { };
   virtual BYTE GetKind() const = 0;
   virtual void SetOwner(HWND hWnd, LPVOID pData) = 0;
   virtual LPCTSTR GetName() const = 0;
   virtual void SetEnabled(BOOL bEnable) = 0;
   virtual BOOL IsEnabled() const = 0;
   virtual void SetItemData(LPARAM lParam) = 0;
   virtual LPARAM GetItemData() const = 0;
   virtual void DrawName(PROPERTYDRAWINFO& di) = 0;
   virtual void DrawValue(PROPERTYDRAWINFO& di) = 0;
   virtual HWND CreateInplaceControl(HWND hWnd, const RECT& rc) = 0;
   virtual BOOL Activate(UINT action, LPARAM lParam) = 0;
   virtual BOOL GetDisplayValue(LPTSTR pstr, UINT cchMax) const = 0;
   virtual UINT GetDisplayValueLength() const = 0;
   virtual BOOL GetValue(VARIANT* pValue) const = 0;
   virtual BOOL SetValue(const VARIANT& value) = 0;
   virtual BOOL SetValue(HWND hWnd) = 0;
};
typedef IProperty* HPROPERTY;


// Property control notification structure
typedef struct tagNMPROPERTYITEM 
{
   NMHDR hdr;
   HPROPERTY prop;
} NMPROPERTYITEM, *LPNMPROPERTYITEM;


#endif // __PROPERTYITEM__H
