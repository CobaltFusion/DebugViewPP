#ifndef __PROPERTYITEMIMPL__H
#define __PROPERTYITEMIMPL__H

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CPropertyItemImpl - Property implementations for the Property controls
//
// Written by Bjarke Viksoe (bjarke@viksoe.dk)
// Copyright (c) 2001-2002 Bjarke Viksoe.
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

#ifndef __PROPERTYITEM__H
  #error PropertyItemImpl.h requires PropertyItem.h to be included first
#endif

#ifndef __PROPERTYITEMEDITORS__H
  #error PropertyItemImpl.h requires PropertyItemEditors.h to be included first
#endif

#ifndef __ATLBASE_H__
  #error PropertyItem.h requires atlbase.h to be included first
#endif



/////////////////////////////////////////////////////////////////////////////
// Base CProperty class

class CProperty : public IProperty
{
protected:
   HWND   m_hWndOwner;
   LPTSTR m_pszName;
   bool   m_fEnabled;
   LPARAM m_lParam;

public:
   CProperty(LPCTSTR pstrName, LPARAM lParam) : m_fEnabled(true), m_lParam(lParam), m_hWndOwner(NULL)
   {
      ATLASSERT(!::IsBadStringPtr(pstrName,-1));
      ATLTRY( m_pszName = new TCHAR[ (::lstrlen(pstrName) * sizeof(TCHAR)) + 1 ] );
      ATLASSERT(m_pszName);
      ::lstrcpy(m_pszName, pstrName);
   }

   virtual ~CProperty()
   {
      delete [] m_pszName;
   }

   virtual void SetOwner(HWND hWnd, LPVOID /*pData*/)
   {
      ATLASSERT(::IsWindow(hWnd));
      ATLASSERT(m_hWndOwner==NULL); // Cannot set it twice
      m_hWndOwner = hWnd;
   }

   virtual LPCTSTR GetName() const
   {
      return m_pszName; // Dangerous!
   }

   virtual void SetEnabled(BOOL bEnable)
   {
      m_fEnabled = (bEnable == TRUE);
   }

   virtual BOOL IsEnabled() const
   {
      return m_fEnabled;
   }

   virtual void SetItemData(LPARAM lParam)
   {
      m_lParam = lParam;
   }

   virtual LPARAM GetItemData() const
   {
      return m_lParam;
   }

   virtual void DrawName(PROPERTYDRAWINFO& di)
   {
      CDCHandle dc(di.hDC);
      COLORREF clrBack, clrFront;
      if( (di.state & ODS_DISABLED) != 0 ) {
         clrFront = di.clrDisabled;
         clrBack = di.clrBack;
      }
      else if( (di.state & ODS_SELECTED) != 0 ) {
         clrFront = di.clrSelText;
         clrBack = di.clrSelBack;
      }
      else {
         clrFront = di.clrText;
         clrBack = di.clrBack;
      }
      RECT rcItem = di.rcItem;
      dc.FillSolidRect(&rcItem, clrBack);
      rcItem.left += 2; // Indent text
      dc.SetBkMode(TRANSPARENT);
      dc.SetBkColor(clrBack);
      dc.SetTextColor(clrFront);
      dc.DrawText(m_pszName, -1, &rcItem, DT_LEFT | DT_SINGLELINE | DT_EDITCONTROL | DT_NOPREFIX | DT_VCENTER);
   }

   virtual void DrawValue(PROPERTYDRAWINFO& /*di*/) 
   { 
   }

   virtual HWND CreateInplaceControl(HWND /*hWnd*/, const RECT& /*rc*/) 
   { 
      return NULL; 
   }

   virtual BOOL Activate(UINT /*action*/, LPARAM /*lParam*/) 
   { 
      return TRUE; 
   }

   virtual BOOL GetDisplayValue(LPTSTR /*pstr*/, UINT /*cchMax*/) const 
   { 
      return FALSE; 
   }

   virtual UINT GetDisplayValueLength() const 
   { 
      return 0; 
   }

   virtual BOOL GetValue(VARIANT* /*pValue*/) const 
   { 
      return FALSE; 
   }

   virtual BOOL SetValue(const VARIANT& /*value*/) 
   { 
      ATLASSERT(false);
      return FALSE; 
   }

   virtual BOOL SetValue(HWND /*hWnd*/) 
   { 
      ATLASSERT(false);
      return FALSE; 
   }
};


/////////////////////////////////////////////////////////////////////////////
// Simple property (displays text)

class CPropertyItem : public CProperty
{
protected:
   CComVariant m_val;

public:
   CPropertyItem(LPCTSTR pstrName, LPARAM lParam) : CProperty(pstrName, lParam)
   {
   }

   BYTE GetKind() const 
   { 
      return PROPKIND_SIMPLE; 
   }

   void DrawValue(PROPERTYDRAWINFO& di)
   {
      UINT cchMax = GetDisplayValueLength() + 1;
      LPTSTR pszText = (LPTSTR) _alloca(cchMax * sizeof(TCHAR));
      ATLASSERT(pszText);
      if( !GetDisplayValue(pszText, cchMax) ) return;
      CDCHandle dc(di.hDC);
      dc.SetBkMode(TRANSPARENT);
      dc.SetTextColor((di.state & ODS_DISABLED) != 0 ? di.clrDisabled : di.clrText);
      dc.SetBkColor(di.clrBack);
      RECT rcText = di.rcItem;
      rcText.left += PROP_TEXT_INDENT;
      dc.DrawText(pszText, -1, &rcText, DT_LEFT | DT_SINGLELINE | DT_EDITCONTROL | DT_NOPREFIX | DT_END_ELLIPSIS | DT_VCENTER);
   }

   BOOL GetDisplayValue(LPTSTR pstr, UINT cchMax) const
   {      
      ATLASSERT(!::IsBadStringPtr(pstr, cchMax));
      // Convert VARIANT to string and use that as display string...
      CComVariant v;
      if( FAILED( v.ChangeType(VT_BSTR, &m_val) ) ) return FALSE;
      USES_CONVERSION;
      ::lstrcpyn(pstr, OLE2CT(v.bstrVal), cchMax);
      return TRUE;
   }

   UINT GetDisplayValueLength() const
   {
      // Hmm, need to convert it to display string first and
      // then take the length...
      // TODO: Call GetDisplayValue() instead.
      CComVariant v;
      if( FAILED( v.ChangeType(VT_BSTR, &m_val) ) ) return 0;
      USES_CONVERSION;
      return v.bstrVal == NULL ? 0 : ::lstrlen(OLE2CT(v.bstrVal));
   }

   BOOL GetValue(VARIANT* pVal) const
   {
      return SUCCEEDED( CComVariant(m_val).Detach(pVal) );
   }

   BOOL SetValue(const VARIANT& value)
   {
      m_val = value;
      return TRUE;
   }
};


/////////////////////////////////////////////////////////////////////////////
// ReadOnly property (with enhanced display features)

class CPropertyReadOnlyItem : public CPropertyItem
{
protected:
   UINT m_uStyle;
   HICON m_hIcon;
   COLORREF m_clrBack;
   COLORREF m_clrText;

public:
   CPropertyReadOnlyItem(LPCTSTR pstrName, LPARAM lParam) : 
      CPropertyItem(pstrName, lParam), 
      m_uStyle( DT_LEFT | DT_SINGLELINE | DT_EDITCONTROL | DT_NOPREFIX | DT_END_ELLIPSIS | DT_VCENTER ),
      m_clrBack(CLR_INVALID),
      m_clrText(CLR_INVALID),
      m_hIcon(NULL)
   {
   }

   CPropertyReadOnlyItem(LPCTSTR pstrName, LPCTSTR pstrValue, LPARAM lParam) : 
      CPropertyItem(pstrName, lParam), 
      m_uStyle( DT_LEFT | DT_SINGLELINE | DT_EDITCONTROL | DT_NOPREFIX | DT_END_ELLIPSIS | DT_VCENTER ),
      m_clrBack(CLR_INVALID),
      m_clrText(CLR_INVALID),
      m_hIcon(NULL)
   {
      m_val = pstrValue;
   }

   void DrawValue(PROPERTYDRAWINFO& di)
   {
      // Get property text
      UINT cchMax = GetDisplayValueLength() + 1;
      LPTSTR pszText = (LPTSTR) _alloca(cchMax * sizeof(TCHAR));
      ATLASSERT(pszText);
      if( !GetDisplayValue(pszText, cchMax) ) return;
      // Prepare paint
      RECT rcText = di.rcItem;
      CDCHandle dc(di.hDC);
      dc.SetBkMode(OPAQUE);
      // Set background color
      COLORREF clrBack = di.clrBack;
      if( m_clrBack != CLR_INVALID ) clrBack = m_clrBack;
      dc.SetBkColor(clrBack);
      // Set text color
      COLORREF clrText = di.clrText;
      if( m_clrText != CLR_INVALID ) clrText = m_clrText;
      if( (di.state & ODS_DISABLED) != 0 ) clrText = di.clrDisabled; 
      dc.SetTextColor(clrText);
      // Draw icon if available
      if( m_hIcon ) {
         POINT pt = { rcText.left + 2, rcText.top + 2 };
         SIZE sizeIcon = { ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON) };
         ::DrawIconEx(dc, pt.x, pt.y, m_hIcon, sizeIcon.cx, sizeIcon.cy, 0, NULL, DI_NORMAL);
         rcText.left += sizeIcon.cx + 4;
      }
      // Draw text with custom style
      rcText.left += PROP_TEXT_INDENT;
      dc.DrawText(pszText, -1, &rcText, m_uStyle);
   }

   // Operations

   // NOTE: To use these methods, you must cast the HPROPERTY 
   //       handle back to the CPropertyReadOnlyItem class.
   //       Nasty stuff, but so far I've settled with this approach.

   COLORREF SetBkColor(COLORREF clrBack)
   {
      COLORREF clrOld = m_clrBack;
      m_clrBack = clrBack;
      return clrOld;
   }

   COLORREF SetTextColor(COLORREF clrText)
   {
      COLORREF clrOld = m_clrText;
      m_clrText = clrText;
      return clrOld;
   }

   HICON SetIcon(HICON hIcon)
   {
      HICON hOldIcon = m_hIcon;
      m_hIcon = hIcon;
      return hOldIcon;
   }

   void ModifyDrawStyle(UINT uRemove, UINT uAdd)
   {
      m_uStyle = (m_uStyle & ~uRemove) | uAdd;
   }
};


/////////////////////////////////////////////////////////////////////////////
// Simple Value property

class CPropertyEditItem : public CPropertyItem
{
protected:
   HWND m_hwndEdit;

public:
   CPropertyEditItem(LPCTSTR pstrName, LPARAM lParam) : 
      CPropertyItem(pstrName, lParam), 
      m_hwndEdit(NULL)
   {
   }

   CPropertyEditItem(LPCTSTR pstrName, CComVariant vValue, LPARAM lParam) : 
      CPropertyItem(pstrName, lParam), 
      m_hwndEdit(NULL)
   {
      m_val = vValue;
   }

   BYTE GetKind() const 
   { 
      return PROPKIND_EDIT; 
   }

   HWND CreateInplaceControl(HWND hWnd, const RECT& rc) 
   {
      // Get default text
      UINT cchMax = GetDisplayValueLength() + 1;
      LPTSTR pszText = (LPTSTR) _alloca(cchMax * sizeof(TCHAR));
      ATLASSERT(pszText);
      if( !GetDisplayValue(pszText, cchMax) ) return NULL;
      // Create EDIT control
      CPropertyEditWindow* win = new CPropertyEditWindow();
      ATLASSERT(win);
      RECT rcWin = rc;
      m_hwndEdit = win->Create(hWnd, rcWin, pszText, WS_VISIBLE | WS_CHILD | ES_LEFT | ES_AUTOHSCROLL);
      ATLASSERT(::IsWindow(m_hwndEdit));
      // Simple hack to validate numbers
      switch( m_val.vt ) {
      case VT_UI1:
      case VT_UI2:
      case VT_UI4:
         win->ModifyStyle(0, ES_NUMBER);
         break;
      }
      return m_hwndEdit;
   }

   BOOL SetValue(const VARIANT& value)
   {
      if( m_val.vt == VT_EMPTY ) m_val = value;
      return SUCCEEDED( m_val.ChangeType(m_val.vt, &value) );
   }

   BOOL SetValue(HWND hWnd) 
   { 
      ATLASSERT(::IsWindow(hWnd));
      int len = ::GetWindowTextLength(hWnd) + 1;
      LPTSTR pstr = (LPTSTR) _alloca(len * sizeof(TCHAR));
      ATLASSERT(pstr);
      if( ::GetWindowText(hWnd, pstr, len) == 0 ) {
         // Bah, an empty string *and* an error causes the same return code!
         if( ::GetLastError() != ERROR_SUCCESS ) return FALSE;
      }
      return SetValue(CComVariant(pstr));
   }

   BOOL Activate(UINT action, LPARAM /*lParam*/)
   {
      switch( action ) {
      case PACT_TAB:
      case PACT_SPACE:
      case PACT_DBLCLICK:
         if( ::IsWindow(m_hwndEdit) ) {
            ::SetFocus(m_hwndEdit);
            ::SendMessage(m_hwndEdit, EM_SETSEL, 0, -1);
         }
         break;
      }
      return TRUE;
   }
};


/////////////////////////////////////////////////////////////////////////////
// Simple Value property

class CPropertyDateItem : public CPropertyEditItem
{
public:
   CPropertyDateItem(LPCTSTR pstrName, LPARAM lParam) : 
      CPropertyEditItem(pstrName, lParam)
   {
   }

   CPropertyDateItem(LPCTSTR pstrName, const SYSTEMTIME stValue, LPARAM lParam) : 
      CPropertyEditItem(pstrName, lParam)
   {
      m_val.vt = VT_DATE;
      m_val.date = 0.0;   // Clear value in case of conversion error below!
      if( stValue.wYear > 0 ) ::SystemTimeToVariantTime(const_cast<SYSTEMTIME*>(&stValue), &m_val.date);
   }

   HWND CreateInplaceControl(HWND hWnd, const RECT& rc) 
   {
      // Get default text
      UINT cchMax = GetDisplayValueLength() + 1;
      LPTSTR pszText = (LPTSTR) _alloca(cchMax * sizeof(TCHAR));
      ATLASSERT(pszText);
      if( !GetDisplayValue(pszText, cchMax) ) return NULL;
      // Create window
      CPropertyDateWindow* win = new CPropertyDateWindow();
      ATLASSERT(win);
      RECT rcWin = rc;
      m_hwndEdit = win->Create(hWnd, rcWin, pszText);
      ATLASSERT(win->IsWindow());
      return *win;
   }

   BOOL GetDisplayValue(LPTSTR pstr, UINT cchMax) const
   {
      if( m_val.date == 0.0 ) {
         ::lstrcpy(pstr, _T(""));
         return TRUE;
      }
      return CPropertyEditItem::GetDisplayValue(pstr, cchMax);
   }

   BOOL SetValue(const VARIANT& value)
   {
      if( value.vt == VT_BSTR && ::SysStringLen(value.bstrVal) == 0 ) {
         m_val.date = 0.0;
         return TRUE;
      }
      return CPropertyEditItem::SetValue(value);
   }

   BOOL SetValue(HWND hWnd)
   {
      if( ::GetWindowTextLength(hWnd) == 0 ) {
         m_val.date = 0.0;
         return TRUE;
      }
      return CPropertyEditItem::SetValue(hWnd);
   }
};


/////////////////////////////////////////////////////////////////////////////
// Checkmark button

class CPropertyCheckButtonItem : public CProperty
{
protected:
   bool m_bValue;

public:
   CPropertyCheckButtonItem(LPCTSTR pstrName, LPARAM lParam) : 
      CProperty(pstrName, lParam),
      m_bValue(false)
   {
   }

   CPropertyCheckButtonItem(LPCTSTR pstrName, bool bValue, LPARAM lParam) : 
      CProperty(pstrName, lParam),
      m_bValue(bValue)
   {
   }

   BYTE GetKind() const 
   { 
      return PROPKIND_CHECK; 
   }

   BOOL GetValue(VARIANT* pVal) const
   {
      return SUCCEEDED( CComVariant(m_bValue).Detach(pVal) );
   }

   BOOL SetValue(const VARIANT& value)
   {
      // Set a new value
      switch( value.vt ) {
      case VT_BOOL:
         m_bValue = (value.boolVal != VARIANT_FALSE);
         return TRUE;
      default:
         ATLASSERT(false);
         return FALSE;
      }
   }

   void DrawValue(PROPERTYDRAWINFO& di)
   {
      int cxThumb = ::GetSystemMetrics(SM_CXMENUCHECK);
      int cyThumb = ::GetSystemMetrics(SM_CYMENUCHECK);
      RECT rcMark = di.rcItem;
      rcMark.left += 10;
      rcMark.right = rcMark.left + cxThumb;
      rcMark.top += 2;
      if( rcMark.top + cyThumb >= rcMark.bottom ) rcMark.top -= rcMark.top + cyThumb - rcMark.bottom + 1;
      rcMark.bottom = rcMark.top + cyThumb;
      UINT uState = DFCS_BUTTONCHECK | DFCS_FLAT;
      if( m_bValue ) uState |= DFCS_CHECKED;
      if( (di.state & ODS_DISABLED) != 0 ) uState |= DFCS_INACTIVE;
      ::DrawFrameControl(di.hDC, &rcMark, DFC_BUTTON, uState);
   }

   BOOL Activate(UINT action, LPARAM /*lParam*/) 
   { 
      switch( action ) {
      case PACT_SPACE:
      case PACT_CLICK:
      case PACT_DBLCLICK:
         if( IsEnabled() ) {
            CComVariant v = !m_bValue;
            ::SendMessage(m_hWndOwner, WM_USER_PROP_CHANGEDPROPERTY, (WPARAM) (VARIANT*) &v, (LPARAM) this);
         }
         break;
      }
      return TRUE;
   }
};


/////////////////////////////////////////////////////////////////////////////
// FileName property

class CPropertyFileNameItem : public CPropertyItem
{
public:
   CPropertyFileNameItem(LPCTSTR pstrName, LPARAM lParam) : CPropertyItem(pstrName, lParam)
   {
   }

   CPropertyFileNameItem(LPCTSTR pstrName, LPCTSTR pstrFilename, LPARAM lParam) : CPropertyItem(pstrName, lParam)
   {
      m_val = pstrFilename;
   }

   HWND CreateInplaceControl(HWND hWnd, const RECT& rc) 
   {
      // Get default text
      TCHAR szText[MAX_PATH] = { 0 };
      if( !GetDisplayValue(szText, (sizeof(szText) / sizeof(TCHAR)) - 1) ) return NULL;      
      // Create EDIT control
      CPropertyButtonWindow* win = new CPropertyButtonWindow();
      ATLASSERT(win);
      RECT rcWin = rc;
      win->m_prop = this;
      win->Create(hWnd, rcWin, szText);
      ATLASSERT(win->IsWindow());
      return *win;
   }

   BOOL SetValue(const VARIANT& value)
   {
      ATLASSERT(V_VT(&value)==VT_BSTR);
      m_val = value;
      return TRUE;
   }

   BOOL SetValue(HWND /*hWnd*/) 
   {
      // Do nothing... A value should be set on reacting to the button notification.
      // In other words: Use SetItemValue() in response to the PLN_BROWSE notification!
      return TRUE;
   }

   BOOL Activate(UINT action, LPARAM /*lParam*/)
   {
      switch( action ) {
      case PACT_BROWSE:
      case PACT_DBLCLICK:
         // Let control owner know
         NMPROPERTYITEM nmh = { m_hWndOwner, ::GetDlgCtrlID(m_hWndOwner), PIN_BROWSE, this };
         ::SendMessage(::GetParent(m_hWndOwner), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh);
         break;
      }
      return TRUE;
   }

   BOOL GetDisplayValue(LPTSTR pstr, UINT cchMax) const
   {
      ATLASSERT(!::IsBadStringPtr(pstr, cchMax));
      *pstr = _T('\0');
      if( m_val.bstrVal == NULL ) return TRUE;
      // Only display actual filename (strip path)
      USES_CONVERSION;
      LPCTSTR pstrFileName = OLE2CT(m_val.bstrVal);
      LPCTSTR p = pstrFileName;
      while( *p != '\0' ) {
         if( *p == _T(':') || *p == _T('\\') ) pstrFileName = p + 1;
         p = ::CharNext(p);
      }
      ::lstrcpyn(pstr, pstrFileName, cchMax);
      return TRUE;
   }

   UINT GetDisplayValueLength() const
   {
      TCHAR szPath[MAX_PATH] = { 0 };
      if( !GetDisplayValue(szPath, (sizeof(szPath) / sizeof(TCHAR)) - 1) ) return 0;
      return ::lstrlen(szPath);
   }
};


/////////////////////////////////////////////////////////////////////////////
// DropDown List property

class CPropertyListItem : public CPropertyItem
{
protected:
   CSimpleArray<CComBSTR> m_arrList;
   HWND m_hwndCombo;

public:
   CPropertyListItem(LPCTSTR pstrName, LPARAM lParam) : 
      CPropertyItem(pstrName, lParam), 
      m_hwndCombo(NULL)
   {
      m_val = -1L;
   }

   CPropertyListItem(LPCTSTR pstrName, LPCTSTR* ppList, int iValue, LPARAM lParam) : 
      CPropertyItem(pstrName, lParam), 
      m_hwndCombo(NULL)
   {
      m_val = -1L;
      if( ppList != NULL ) {
         SetList(ppList);
         SetValue(CComVariant(iValue));
      }
   }

   BYTE GetKind() const 
   { 
      return PROPKIND_LIST; 
   }

   HWND CreateInplaceControl(HWND hWnd, const RECT& rc) 
   {
      // Get default text
      UINT cchMax = GetDisplayValueLength() + 1;
      LPTSTR pszText = (LPTSTR) _alloca(cchMax * sizeof(TCHAR));
      ATLASSERT(pszText);
      if( !GetDisplayValue(pszText, cchMax) ) return NULL;
      // Create 'faked' DropDown control
      CPropertyListWindow* win = new CPropertyListWindow();
      ATLASSERT(win);
      RECT rcWin = rc;
      m_hwndCombo = win->Create(hWnd, rcWin, pszText);
      ATLASSERT(win->IsWindow());
      // Add to list
      USES_CONVERSION;      
      for( int i = 0; i < m_arrList.GetSize(); i++ ) win->AddItem(OLE2CT(m_arrList[i]));
      win->SelectItem(m_val.lVal);
      // Go...
      return *win;
   }

   BOOL Activate(UINT action, LPARAM /*lParam*/)
   {
      switch( action ) {
      case PACT_SPACE:
         if( ::IsWindow(m_hwndCombo) ) {
            // Fake button click...
            ::SendMessage(m_hwndCombo, WM_COMMAND, MAKEWPARAM(0, BN_CLICKED), 0);
         }
         break;
      case PACT_DBLCLICK:
         // Simulate neat VB control effect. DblClick cycles items in list.
         // Set value and recycle edit control
         if( IsEnabled() ) {
            CComVariant v = m_val.lVal + 1L;
            ::SendMessage(m_hWndOwner, WM_USER_PROP_CHANGEDPROPERTY, (WPARAM) (VARIANT*) &v, (LPARAM) this);
         }
         break;
      }
      return TRUE;
   }

   BOOL GetDisplayValue(LPTSTR pstr, UINT cchMax) const
   {
      ATLASSERT(m_val.vt==VT_I4);
      ATLASSERT(!::IsBadStringPtr(pstr, cchMax));
      *pstr = _T('\0');
      if( m_val.lVal < 0 || m_val.lVal >= m_arrList.GetSize() ) return FALSE;
      USES_CONVERSION;
      ::lstrcpyn( pstr, OLE2CT(m_arrList[m_val.lVal]), cchMax) ;
      return TRUE;
   }

   UINT GetDisplayValueLength() const
   {
      ATLASSERT(m_val.vt==VT_I4);
      if( m_val.lVal < 0 || m_val.lVal >= m_arrList.GetSize() ) return 0;
      BSTR bstr = m_arrList[m_val.lVal];
      USES_CONVERSION;
      return bstr == NULL ? 0 : ::lstrlen(OLE2CT(bstr));
   }

   BOOL SetValue(const VARIANT& value)
   {
      switch( value.vt ) {
      case VT_BSTR:
         {
            m_val = 0L;
            for( int i = 0; i < m_arrList.GetSize(); i++ ) {
               if( ::wcscmp(value.bstrVal, m_arrList[i]) == 0 ) {
                  m_val = (long) i;
                  return TRUE;
               }
            }
            return FALSE;
         }
         break;
      default:
         // Treat as index into list
         if( FAILED( m_val.ChangeType(VT_I4, &value) ) ) return FALSE;
         if( m_val.lVal >= m_arrList.GetSize() ) m_val.lVal = 0L;
         return TRUE;
      }
   }

   BOOL SetValue(HWND hWnd)
   { 
      ATLASSERT(::IsWindow(hWnd));
      int len = ::GetWindowTextLength(hWnd) + 1;
      LPTSTR pstr = (LPTSTR) _alloca(len * sizeof(TCHAR));
      ATLASSERT(pstr);
      if( !::GetWindowText(hWnd, pstr, len) ) {
         if( ::GetLastError() != ERROR_SUCCESS ) return FALSE;
      }
      return SetValue(CComVariant(pstr));
   }

   void SetList(LPCTSTR* ppList)
   {
      ATLASSERT(ppList);
      m_arrList.RemoveAll();
      while( *ppList != NULL ) {
         CComBSTR bstr(*ppList);
         m_arrList.Add(bstr);
         ppList++;
      }
      if( m_val.lVal < 0L ) m_val = 0L;
      if( m_val.lVal >= (LONG) m_arrList.GetSize() ) m_val = 0L;
   }

   void AddListItem(LPCTSTR pstrText)
   {
      ATLASSERT(!::IsBadStringPtr(pstrText,-1));
      CComBSTR bstr(pstrText);
      m_arrList.Add(bstr);
      if( m_val.lVal < 0L ) m_val = 0L;
   }
};


/////////////////////////////////////////////////////////////////////////////
// Boolean property

class CPropertyBooleanItem : public CPropertyListItem
{
public:
   CPropertyBooleanItem(LPCTSTR pstrName, LPARAM lParam) : CPropertyListItem(pstrName, lParam)
   {
      _InitBooleanList();
   }

   CPropertyBooleanItem(LPCTSTR pstrName, bool bValue, LPARAM lParam) : CPropertyListItem(pstrName, lParam)
   {
      _InitBooleanList();
      SetValue(CComVariant(bValue));
   }

   BOOL SetValue(const VARIANT& value)
   {
      // Convert to list index...
      if( value.vt == VT_BOOL ) return CPropertyListItem::SetValue(CComVariant(value.boolVal ? 1L : 0L));
      return CPropertyListItem::SetValue(value);
   }

   VOID _InitBooleanList()
   {
#ifdef IDS_TRUE
      TCHAR szBuffer[32] = { 0 };
      ::LoadString(_Module.GetResourceInstance(), IDS_FALSE, szBuffer, (sizeof(szBuffer) / sizeof(TCHAR)) - 1);
      AddListItem(szBuffer);
      ::LoadString(_Module.GetResourceInstance(), IDS_TRUE, szBuffer, (sizeof(szBuffer) / sizeof(TCHAR)) - 1);
      AddListItem(szBuffer);
#else
      AddListItem(_T("False"));
      AddListItem(_T("True"));
#endif  // IDS_TRUE
   }
};


/////////////////////////////////////////////////////////////////////////////
// ListBox (DropDown) Control property

class CPropertyComboItem : public CPropertyItem
{
public:
   CListBox m_ctrl;

   CPropertyComboItem(LPCTSTR pstrName, LPARAM lParam) : 
      CPropertyItem(pstrName, lParam)
   {
   }

   CPropertyComboItem(LPCTSTR pstrName, HWND hWnd, int iValue, LPARAM lParam) : 
      CPropertyItem(pstrName, lParam)
   {
      m_ctrl = hWnd;
      SetValue(CComVariant(iValue));
   }

   HWND CreateInplaceControl(HWND hWnd, const RECT& rc) 
   {
      ATLASSERT(::IsWindow(m_ctrl));
      // Create window
      CPropertyComboWindow* win = new CPropertyComboWindow();
      ATLASSERT(win);
      RECT rcWin = rc;
      win->m_hWndCombo = m_ctrl;
      win->Create(hWnd, rcWin);
      ATLASSERT(::IsWindow(*win));
      return *win;
   }

   BYTE GetKind() const 
   { 
      return PROPKIND_CONTROL; 
   }

   void DrawValue(PROPERTYDRAWINFO& di) 
   { 
      RECT rc = di.rcItem;
      ::InflateRect(&rc, 0, -1);
      DRAWITEMSTRUCT dis = { 0 };
      dis.hDC = di.hDC;
      dis.hwndItem = m_ctrl;
      dis.CtlID = m_ctrl.GetDlgCtrlID();
      dis.CtlType = ODT_LISTBOX;
      dis.rcItem = rc;
      dis.itemState = ODS_DEFAULT | ODS_COMBOBOXEDIT;
      dis.itemID = m_ctrl.GetCurSel();
      dis.itemData = (int) m_ctrl.GetItemData(dis.itemID);
      ::SendMessage(m_ctrl, OCM_DRAWITEM, dis.CtlID, (LPARAM) &dis);
   }

   BOOL GetValue(VARIANT* pValue) const 
   { 
      CComVariant v = (int) m_ctrl.GetItemData(m_ctrl.GetCurSel());
      return SUCCEEDED( v.Detach(pValue) );
   }

   BOOL SetValue(HWND /*hWnd*/) 
   {      
      int iSel = m_ctrl.GetCurSel();
      CComVariant v = (int) m_ctrl.GetItemData(iSel);
      return SetValue(v); 
   }

   BOOL SetValue(const VARIANT& value)
   {
      ATLASSERT(value.vt==VT_I4);
      for( int i = 0; i < m_ctrl.GetCount(); i++ ) {
         if( m_ctrl.GetItemData(i) == (DWORD_PTR) value.lVal ) {
            m_ctrl.SetCurSel(i);
            return TRUE;
         }
      }
      return FALSE;
   }
};


/////////////////////////////////////////////////////////////////////////////
//
// CProperty creators
//

inline HPROPERTY PropCreateVariant(LPCTSTR pstrName, const VARIANT& vValue, LPARAM lParam = 0)
{
   return new CPropertyEditItem(pstrName, vValue, lParam);
}

inline HPROPERTY PropCreateSimple(LPCTSTR pstrName, LPCTSTR pstrValue, LPARAM lParam = 0)
{
   return new CPropertyEditItem(pstrName, CComVariant(pstrValue), lParam);
}

inline HPROPERTY PropCreateSimple(LPCTSTR pstrName, int iValue, LPARAM lParam = 0)
{
   return new CPropertyEditItem(pstrName, CComVariant(iValue), lParam);
}

inline HPROPERTY PropCreateSimple(LPCTSTR pstrName, bool bValue, LPARAM lParam = 0)
{
   return new CPropertyBooleanItem(pstrName, bValue, lParam);
}

inline HPROPERTY PropCreateFileName(LPCTSTR pstrName, LPCTSTR pstrFileName, LPARAM lParam = 0)
{
   return new CPropertyFileNameItem(pstrName, pstrFileName, lParam);
}

inline HPROPERTY PropCreateDate(LPCTSTR pstrName, const SYSTEMTIME stValue, LPARAM lParam = 0)
{
   return new CPropertyDateItem(pstrName, stValue, lParam);
}

inline HPROPERTY PropCreateList(LPCTSTR pstrName, LPCTSTR* ppList, int iValue = 0, LPARAM lParam = 0)
{
   return new CPropertyListItem(pstrName, ppList, iValue, lParam);
}

inline HPROPERTY PropCreateComboControl(LPCTSTR pstrName, HWND hWndList, int iValue, LPARAM lParam = 0)
{
   return new CPropertyComboItem(pstrName, hWndList, iValue, lParam);
}

inline HPROPERTY PropCreateCheckButton(LPCTSTR pstrName, bool bValue, LPARAM lParam = 0)
{
   return new CPropertyCheckButtonItem(pstrName, bValue, lParam);
}

inline HPROPERTY PropCreateReadOnlyItem(LPCTSTR pstrName, LPCTSTR pstrValue = _T(""), LPARAM lParam = 0)
{
   return new CPropertyReadOnlyItem(pstrName, pstrValue, lParam);
}


#endif // __PROPERTYITEMIMPL__H
