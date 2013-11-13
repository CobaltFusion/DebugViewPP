#ifndef __PROPERTYITEMEDITORS__H
#define __PROPERTYITEMEDITORS__H

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CPropertyItemEditors - Editors for Property controls
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

#ifndef __PROPERTYITEM__H
  #error PropertyItemEditors.h requires PropertyItem.h to be included first
#endif

#define PROP_TEXT_INDENT 2


/////////////////////////////////////////////////////////////////////////////
// Plain editor with a EDIT box

class CPropertyEditWindow : 
   public CWindowImpl< CPropertyEditWindow, CEdit, CControlWinTraits >
{
public:
   DECLARE_WND_SUPERCLASS(_T("WTL_InplacePropertyEdit"), CEdit::GetWndClassName())

   bool m_fCancel;

   CPropertyEditWindow() : m_fCancel(false)
   {
   }

   virtual void OnFinalMessage(HWND /*hWnd*/)
   {
      delete this;
   }

   BEGIN_MSG_MAP(CPropertyEditWindow)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
      MESSAGE_HANDLER(WM_CHAR, OnChar)
      MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
      MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
      MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      LRESULT lRes = DefWindowProc();
      SetFont( CWindow(GetParent()).GetFont() );
      SetMargins(PROP_TEXT_INDENT, 0);   // Force EDIT margins so text doesn't jump
      return lRes;
   }

   LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
   {
      switch( wParam ) {
      case VK_ESCAPE:
         m_fCancel = true;
         // FALL THROUGH...
      case VK_RETURN:
         // Force focus to parent to update value (see OnKillFocus()...)
         ::SetFocus(GetParent());
         // FIX: Allowing a multiline EDIT to VK_ESCAPE will send a WM_CLOSE
         //      to the list control if it's embedded in a dialog!?
         return 0;
      case VK_TAB:
      case VK_UP:
      case VK_DOWN:
         return ::PostMessage(GetParent(), WM_USER_PROP_NAVIGATE, LOWORD(wParam), 0);
      case VK_LEFT:
         int lLow, lHigh;
         GetSel(lLow, lHigh);
         if( lLow != lHigh || lLow != 0 ) break;
         return ::PostMessage(GetParent(), WM_USER_PROP_NAVIGATE, LOWORD(wParam), 0);
      case VK_RIGHT:
         GetSel(lLow, lHigh);
         if( lLow != lHigh || lLow != GetWindowTextLength() ) break;
         return ::PostMessage(GetParent(), WM_USER_PROP_NAVIGATE, LOWORD(wParam), 0);
      }
      bHandled = FALSE;
      return 0;
   }

   LRESULT OnChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
   {
      switch( LOWORD(wParam) ) {
      case VK_RETURN:
      case VK_ESCAPE:
         // Do not BEEP!!!!
         return 0;
      }
      bHandled = FALSE;
      return 0;
   }

   LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {
      m_fCancel = false;
      bHandled = FALSE;
      return 0;
   }

   LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
   {
      LRESULT lRes = DefWindowProc(uMsg, wParam, lParam);
      m_fCancel |= (GetModify() == FALSE);
      ::SendMessage(GetParent(), m_fCancel ? WM_USER_PROP_CANCELPROPERTY : WM_USER_PROP_UPDATEPROPERTY, 0, (LPARAM) m_hWnd);
      return lRes;
   }

   LRESULT OnGetDlgCode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
   {
      return DefWindowProc(uMsg, wParam, lParam) | DLGC_WANTALLKEYS | DLGC_WANTARROWS;
   }
};


/////////////////////////////////////////////////////////////////////////////
// General implementation of editor with button

template< class T, class TBase = CEdit >
class CPropertyDropWindowImpl : 
   public CWindowImpl< T, TBase, CControlWinTraits >
{
public:
   DECLARE_WND_SUPERCLASS(NULL, TBase::GetWndClassName())

   CContainedWindowT<CButton> m_wndButton;
   bool m_bReadOnly;

   virtual void OnFinalMessage(HWND /*hWnd*/)
   {
      delete (T*) this;
   }

   typedef CPropertyDropWindowImpl< T > thisClass;

   BEGIN_MSG_MAP(thisClass)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
      MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
      MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
      MESSAGE_HANDLER(WM_CHAR, OnChar)
      MESSAGE_HANDLER(WM_LBUTTONDOWN, OnMouseButtonClick)
      MESSAGE_HANDLER(WM_RBUTTONDOWN, OnMouseButtonClick)
      MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
   ALT_MSG_MAP(1) // Button
      MESSAGE_HANDLER(WM_KEYDOWN, OnButtonKeyDown)
      MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
   {
      LRESULT lRes = DefWindowProc(uMsg, wParam, lParam);
      RECT rcClient = { 0 };
      GetClientRect(&rcClient);
      int cy = rcClient.bottom - rcClient.top;
      // Setup EDIT control
      SetFont( CWindow(GetParent()).GetFont() );
      ModifyStyle(WS_BORDER, ES_LEFT);
      SendMessage(EM_SETMARGINS, EC_LEFTMARGIN|EC_RIGHTMARGIN, MAKELONG(PROP_TEXT_INDENT, ::GetSystemMetrics(SM_CXVSCROLL)));
      // Create button
      RECT rcButton = { rcClient.right - cy, rcClient.top - 1, rcClient.right, rcClient.bottom };
      m_wndButton.Create(this, 1, m_hWnd, &rcButton, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_PUSHBUTTON | BS_OWNERDRAW);
      ATLASSERT(m_wndButton.IsWindow());
      m_wndButton.SetFont(GetFont());
      // HACK: Windows needs to repaint this guy again!
      m_wndButton.SetFocus();
      m_bReadOnly = true;
      return lRes;
   }

   LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {
      if( !m_bReadOnly ) {
         bHandled = FALSE;
      }
      else {
         // Set focus to button to prevent input
         m_wndButton.SetFocus();
         m_wndButton.Invalidate();
      }
      return 0;
   }

   LRESULT OnKillFocus(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
   {
      if( (HWND) wParam != m_wndButton ) ::SendMessage(GetParent(), WM_USER_PROP_UPDATEPROPERTY, 0, (LPARAM) m_hWnd);
      bHandled = FALSE;
      return 0;
   }

   LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
   {
      if( m_bReadOnly ) {
         bHandled = FALSE;
         return 0;
      }
      switch( wParam ) {
      case VK_F2:
      case VK_F4:
      case VK_SPACE:
         m_wndButton.Click();
         return 0;
      case VK_RETURN:
      case VK_ESCAPE:
         // Announce the new value
         ::PostMessage(GetParent(), wParam == VK_RETURN ? WM_USER_PROP_UPDATEPROPERTY : WM_USER_PROP_CANCELPROPERTY, 0, (LPARAM) m_hWnd);
         ::SetFocus(GetParent());
         break;
      case VK_TAB:
      case VK_UP:
      case VK_DOWN:
         return ::PostMessage(GetParent(), WM_USER_PROP_NAVIGATE, LOWORD(wParam), 0);
      case VK_LEFT:
         int lLow, lHigh;
         SendMessage(EM_GETSEL, (WPARAM) &lLow, (LPARAM) &lHigh);
         if( lLow != lHigh || lLow != 0 ) break;
         return ::PostMessage(GetParent(), WM_USER_PROP_NAVIGATE, LOWORD(wParam), 0);
      case VK_RIGHT:
         SendMessage(EM_GETSEL, (WPARAM) &lLow, (LPARAM) &lHigh);
         if( lLow != lHigh || lLow != GetWindowTextLength() ) break;
         return ::PostMessage(GetParent(), WM_USER_PROP_NAVIGATE, LOWORD(wParam), 0);
      }
      bHandled = FALSE;
      return 0;
   }

   LRESULT OnChar(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {
      // Don't allow any editing
      if( !m_bReadOnly ) bHandled = FALSE;
      return 0;
   }

   LRESULT OnMouseButtonClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {
      // Don't allow selection or context menu for edit box
      if( !m_bReadOnly ) bHandled = FALSE;
      return 0;
   }

   // Button

   LRESULT OnButtonKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
   {
      switch( wParam ) {
      case VK_UP:
      case VK_DOWN:
         return ::PostMessage(GetParent(), WM_USER_PROP_NAVIGATE, LOWORD(wParam), 0);
      case VK_F2:
      case VK_F4:
      case VK_SPACE:
         m_wndButton.Click();
         return 0;
      case VK_ESCAPE:
         ::PostMessage(GetParent(), WM_USER_PROP_UPDATEPROPERTY, 0, (LPARAM) m_hWnd);
         return 0;
      }
      bHandled = FALSE;
      return 0;
   }

   LRESULT OnGetDlgCode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
   {
      return DefWindowProc(uMsg, wParam, lParam) | DLGC_WANTALLKEYS;
   }
};


/////////////////////////////////////////////////////////////////////////////
// Editor with calendar dropdown

class CPropertyDateWindow : 
   public CPropertyDropWindowImpl<CPropertyDateWindow>
{
public:
   DECLARE_WND_SUPERCLASS(_T("WTL_InplacePropertyDateEdit"), CEdit::GetWndClassName())

   CContainedWindowT<CMonthCalendarCtrl> m_wndCalendar;

   typedef CPropertyDropWindowImpl<CPropertyDateWindow> baseClass;

   BEGIN_MSG_MAP(CPropertyDateWindow)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
      COMMAND_CODE_HANDLER(BN_CLICKED, OnButtonClicked)
      NOTIFY_CODE_HANDLER(MCN_SELECT, OnDateSelect)
      CHAIN_MSG_MAP( baseClass )
   ALT_MSG_MAP(1) // Button
      CHAIN_MSG_MAP_ALT( baseClass, 1 )
   ALT_MSG_MAP(2) // Calendar
      MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
   END_MSG_MAP()

   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
   {
      LRESULT lRes = baseClass::OnCreate(uMsg, wParam, lParam, bHandled);
      // Create dropdown list (as hidden)
      m_wndCalendar.Create(this, 2, m_hWnd, &rcDefault, NULL, WS_POPUP | WS_BORDER);
      ATLASSERT(m_wndCalendar.IsWindow());
      m_wndCalendar.SetFont(GetFont());
      m_bReadOnly = false;
      return lRes;
   }

   LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {
      if( m_wndCalendar.IsWindow() ) m_wndCalendar.DestroyWindow();
      bHandled = FALSE;
      return 0;
   }

   LRESULT OnButtonClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      // Set selection
      TCHAR szDate[80] = { 0 };
      GetWindowText(szDate, (sizeof(szDate) / sizeof(TCHAR)) - 1);
      CComVariant v = szDate;
      SYSTEMTIME st = { 0 };
      if( SUCCEEDED( v.ChangeType(VT_DATE) ) ) ::VariantTimeToSystemTime(v.date, &st);
      if( st.wYear == 0 ) ::GetLocalTime(&st);
      m_wndCalendar.SetCurSel(&st);
      // Move the calendar under the item
      RECT rcCalendar = { 0 };
      m_wndCalendar.GetMinReqRect(&rcCalendar);
      RECT rcWin = { 0 };
      GetWindowRect(&rcWin);
      ::OffsetRect(&rcCalendar, rcWin.left, rcWin.bottom);
      m_wndCalendar.SetWindowPos(HWND_TOPMOST, &rcCalendar, SWP_SHOWWINDOW);
      m_wndCalendar.SetFocus();
      return 0;
   }

   LRESULT OnDateSelect(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
   {
      USES_CONVERSION;
      SYSTEMTIME st = { 0 };
      m_wndCalendar.GetCurSel(&st);
      st.wHour = st.wMinute = st.wSecond = st.wMilliseconds = 0;
      CComVariant v;
      v.vt = VT_DATE;
      v.date = 0.0;
      if( st.wYear > 0 ) ::SystemTimeToVariantTime(&st, &v.date);
      v.ChangeType(VT_BSTR);
      SetWindowText(OLE2CT(v.bstrVal));
      SetFocus();
      return 0;
   }

   // Calendar message handlers
   
   LRESULT OnKillFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      LRESULT lRes = m_wndCalendar.DefWindowProc();
      m_wndCalendar.ShowWindow(SW_HIDE);
      return lRes;
   }

   // Ownerdrawn button message handler

   LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
   {
      LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) lParam;
      if( m_wndButton != lpdis->hwndItem ) return 0;
      CDCHandle dc(lpdis->hDC);
      // Paint as dropdown button
      dc.DrawFrameControl(&lpdis->rcItem, DFC_SCROLL, (lpdis->itemState & ODS_SELECTED) != 0 ? DFCS_SCROLLDOWN | DFCS_PUSHED : DFCS_SCROLLDOWN);
      return 0;
   }
};


/////////////////////////////////////////////////////////////////////////////
// Editor with dropdown list

class CPropertyListWindow : 
   public CPropertyDropWindowImpl<CPropertyListWindow>
{
public:
   DECLARE_WND_SUPERCLASS(_T("WTL_InplacePropertyList"), CEdit::GetWndClassName())

   CContainedWindowT<CListBox> m_wndList;
   int m_cyList;      // Used to resize the listbox when first shown

   typedef CPropertyDropWindowImpl<CPropertyListWindow> baseClass;

   BEGIN_MSG_MAP(CPropertyListWindow)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      MESSAGE_HANDLER(WM_CHAR, OnChar)
      MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
      COMMAND_CODE_HANDLER(BN_CLICKED, OnButtonClicked)
      CHAIN_MSG_MAP(baseClass)
   ALT_MSG_MAP(1) // Button
      CHAIN_MSG_MAP_ALT(baseClass, 1)
   ALT_MSG_MAP(2) // List
      MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
      MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
      MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
   END_MSG_MAP()

   void AddItem(LPCTSTR pstrItem)
   {
      ATLASSERT(m_wndList.IsWindow());
      ATLASSERT(!::IsBadStringPtr(pstrItem,-1));
      m_wndList.AddString(pstrItem);
      m_cyList = 0;
   }

   void SelectItem(int idx)
   {
      ATLASSERT(m_wndList.IsWindow());      
      m_wndList.SetCurSel(idx);
   }

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {
      // Create dropdown list (as hidden)
      RECT rc = CWindow::rcDefault;
      m_wndList.Create(this, 2, m_hWnd, &rc, NULL, WS_POPUP | WS_BORDER | WS_VSCROLL);
      ATLASSERT(m_wndList.IsWindow());
      m_wndList.SetFont( CWindow(GetParent()).GetFont() );
      // Go create the rest of the control...
      bHandled = FALSE;
      return 0;
   }

   LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {
      if( m_wndList.IsWindow() ) m_wndList.DestroyWindow();
      bHandled = FALSE;
      return 0;
   }

   LRESULT OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
   {
      // Let the dropdown-box handle the keypress...
      if( (m_wndList.GetStyle() & WS_VISIBLE) != 0 ) {
         m_wndList.PostMessage(uMsg, wParam, lParam);
      }
      else {
         TCHAR szStr[] = { (TCHAR) wParam, _T('\0') };
         int idx = m_wndList.FindString(-1, szStr);
         if( idx == LB_ERR ) return 0;
         m_wndList.SetCurSel(idx);
         BOOL bDummy = FALSE;
         OnKeyDown(WM_KEYDOWN, VK_RETURN, 0, bDummy);      
      }
      return 0; // Don't allow any editing
   }

   LRESULT OnButtonClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      if( m_cyList == 0 ) {
         // Resize list to fit all items (but not more than 140 pixels)
         const int MAX_HEIGHT = 140;
         int cy = m_wndList.GetCount() * m_wndList.GetItemHeight(0);
         m_cyList = min( MAX_HEIGHT, cy + (::GetSystemMetrics(SM_CYBORDER)*2) );
      }
      // Move the dropdown under the item
      RECT rcWin = { 0 };
      GetWindowRect(&rcWin);
      RECT rc = { rcWin.left, rcWin.bottom, rcWin.right, rcWin.bottom + m_cyList };
      m_wndList.SetWindowPos(HWND_TOPMOST, &rc, SWP_SHOWWINDOW);
      return 0;
   }

   // List message handlers
   
   LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
   {
      switch( wParam ) {
      case VK_RETURN:
         {
            int idx = m_wndList.GetCurSel();
            if( idx >= 0 ) {
               // Copy text from list to item
               int len = m_wndList.GetTextLen(idx) + 1;
               LPTSTR pstr = (LPTSTR) _alloca(len * sizeof(TCHAR));
               m_wndList.GetText(idx, pstr);
               SetWindowText(pstr);
               // Announce the new value
               ::SendMessage(GetParent(), WM_USER_PROP_UPDATEPROPERTY, 0, (LPARAM) m_hWnd);
            }
         }
         ::SetFocus(GetParent());
         break;
      case VK_ESCAPE:
         // Announce the cancellation
         ::SendMessage(GetParent(), WM_USER_PROP_CANCELPROPERTY, 0, (LPARAM) m_hWnd);
         ::SetFocus(GetParent());
         break;
      }
      bHandled = FALSE;
      return 0;
   }

   LRESULT OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      LRESULT lRes = m_wndList.DefWindowProc();
      // Selected an item? Fake RETURN key to copy new value...
      BOOL bDummy = FALSE;
      OnKeyDown(WM_KEYDOWN, VK_RETURN, 0, bDummy);
      return lRes;
   }

   LRESULT OnKillFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      LRESULT lRes = m_wndList.DefWindowProc();
      m_wndList.ShowWindow(SW_HIDE);
      return lRes;
   }

   // Ownerdrawn button message handler

   LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
   {
      LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) lParam;
      if( m_wndButton != lpdis->hwndItem ) return 0;
      CDCHandle dc(lpdis->hDC);
      // Paint as dropdown button
      dc.DrawFrameControl(&lpdis->rcItem, DFC_SCROLL, (lpdis->itemState & ODS_SELECTED) != 0 ? DFCS_SCROLLDOWN | DFCS_PUSHED : DFCS_SCROLLDOWN);
      return 0;
   }
};


/////////////////////////////////////////////////////////////////////////////
// Editor with embedded ListBox control

class CPropertyComboWindow : 
   public CPropertyDropWindowImpl<CPropertyComboWindow, CStatic>
{
public:
   DECLARE_WND_SUPERCLASS(_T("WTL_InplacePropertyCombo"), CStatic::GetWndClassName())

   CContainedWindowT<CListBox> m_wndList;
   HWND m_hWndCombo;  // Listbox supplied by Property class
   int m_cyList;      // Used to resize the listbox when first shown

   typedef CPropertyDropWindowImpl<CPropertyComboWindow, CStatic> baseClass;

   CPropertyComboWindow() : 
      m_wndList(this, 2), 
      m_hWndCombo(NULL), 
      m_cyList(0)
   {
   }

   BEGIN_MSG_MAP(CPropertyComboWindow)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      MESSAGE_HANDLER(WM_PAINT, OnPaint)
      MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
      MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
      COMMAND_CODE_HANDLER(BN_CLICKED, OnButtonClicked)
      CHAIN_MSG_MAP( baseClass )
   ALT_MSG_MAP(1) // Button
      MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
      CHAIN_MSG_MAP_ALT( baseClass, 1 )
   ALT_MSG_MAP(2) // List
      MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {
      ATLASSERT(::IsWindow(m_hWndCombo));      
      m_wndList.SubclassWindow(m_hWndCombo);
      // Go create the rest of the control...
      bHandled = FALSE;
      return 0;
   }

   LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {
      if( ::GetCapture() == m_wndList ) ::ReleaseCapture();
      if( m_wndList.IsWindow() ) m_wndList.UnsubclassWindow();
      if( ::IsWindowVisible(m_hWndCombo) ) ::ShowWindow(m_hWndCombo, SW_HIDE);
      bHandled = FALSE;
      return 0;
   }

   LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      CPaintDC dc( m_hWnd );      
      RECT rcButton = { 0 };
      m_wndButton.GetWindowRect(&rcButton);
      RECT rcClient = { 0 };
      GetClientRect(&rcClient);
      rcClient.right -= rcButton.right - rcButton.left;
      DRAWITEMSTRUCT dis = { 0 };
      dis.hDC = dc;
      dis.hwndItem = m_wndList;
      dis.CtlID = m_wndList.GetDlgCtrlID();
      dis.CtlType = ODT_LISTBOX;
      dis.rcItem = rcClient;
      dis.itemState = ODS_DEFAULT | ODS_COMBOBOXEDIT;
      dis.itemID = m_wndList.GetCurSel();
      dis.itemData = (int) m_wndList.GetItemData(dis.itemID);
      m_wndList.SendMessage(OCM_DRAWITEM, dis.CtlID, (LPARAM) &dis);
      return 0;
   }

   LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
   {
      bHandled = FALSE;
      if( !::IsWindowVisible(m_hWndCombo) ) return 0;
      switch( wParam ) {
      case VK_UP:
      case VK_DOWN:
      case VK_PRIOR:
      case VK_NEXT:
      case VK_HOME:
      case VK_END:
         ::SendMessage(m_hWndCombo, WM_KEYDOWN, wParam, 1L);
         bHandled = TRUE;
         break;
      case VK_ESCAPE:
      case VK_RETURN:
         ::ReleaseCapture();
         m_wndList.ShowWindow(SW_HIDE);
         // Announce the new value
         Invalidate();
         ::PostMessage(GetParent(), WM_USER_PROP_UPDATEPROPERTY, 0, (LPARAM) m_hWnd);
         bHandled = TRUE;
         break;
      }
      return 0;
   }

   LRESULT OnButtonClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      if( m_cyList == 0 ) {
         // Resize list to fit all items (but not more than 140 pixels)
         const int MAX_HEIGHT = 140;
         int cy = m_wndList.GetCount() * m_wndList.GetItemHeight(0);
         m_cyList = min( MAX_HEIGHT, cy + (::GetSystemMetrics(SM_CYBORDER)*2) );
      }
      // Move the dropdown under the item
      RECT rcWin = { 0 };
      GetWindowRect(&rcWin);
      RECT rc = { rcWin.left, rcWin.bottom, rcWin.right, rcWin.bottom + m_cyList };
      m_wndList.SetWindowPos(HWND_TOPMOST, &rc, SWP_SHOWWINDOW);
      m_wndList.SetFocus();
      ::SetCapture(m_wndList);
      return 0;
   }

   // List message handlers

   LRESULT OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
   {      
      LRESULT lRes = m_wndList.DefWindowProc();
      ::ReleaseCapture();
      m_wndList.ShowWindow(SW_HIDE);
      //
      POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
      BOOL bOutside = TRUE;
      int iSel = m_wndList.ItemFromPoint(pt, bOutside);
      if( !bOutside ) {
         m_wndList.SetCurSel(iSel);
         // Announce the new value
         Invalidate();
         ::PostMessage(GetParent(), WM_USER_PROP_UPDATEPROPERTY, 0, (LPARAM) m_hWnd);
      }
      return lRes;
   }

   // Ownerdrawn button message handler

   LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
   {
      LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) lParam;
      if( m_wndButton != lpdis->hwndItem ) return 0;
      CDCHandle dc(lpdis->hDC);
      // Paint as dropdown button
      dc.DrawFrameControl(&lpdis->rcItem, DFC_SCROLL, (lpdis->itemState & ODS_SELECTED) != 0 ? DFCS_SCROLLDOWN | DFCS_PUSHED : DFCS_SCROLLDOWN);
      return 0;
   }
};


/////////////////////////////////////////////////////////////////////////////
// Editor with browse button

class CPropertyButtonWindow : 
   public CPropertyDropWindowImpl<CPropertyButtonWindow>
{
public:
   DECLARE_WND_SUPERCLASS(_T("WTL_InplacePropertyButton"), CEdit::GetWndClassName())

   IProperty* m_prop; // BUG: Dangerous reference

   typedef CPropertyDropWindowImpl<CPropertyButtonWindow> baseClass;

   BEGIN_MSG_MAP(CPropertyButtonWindow)
      COMMAND_CODE_HANDLER(BN_CLICKED, OnButtonClicked)
      MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
      CHAIN_MSG_MAP( baseClass )
   ALT_MSG_MAP(1) // Button
      CHAIN_MSG_MAP_ALT( baseClass, 1 )
   END_MSG_MAP()

   LRESULT OnButtonClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      ATLASSERT(m_prop);
      // Call Property class' implementation of BROWSE action
      m_prop->Activate(PACT_BROWSE, 0);
      // Tell control to update its display
      ::SendMessage(GetParent(), WM_USER_PROP_UPDATEPROPERTY, 0, (LPARAM) m_hWnd);
      return 0;
   }

   LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
   {
      LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) lParam;
      if( m_wndButton != lpdis->hwndItem ) return 0;
      CDCHandle dc(lpdis->hDC);
      // Paint as ellipsis button
      dc.DrawFrameControl(&lpdis->rcItem, DFC_BUTTON, (lpdis->itemState & ODS_SELECTED) != 0 ? DFCS_BUTTONPUSH | DFCS_PUSHED : DFCS_BUTTONPUSH);
      dc.SetBkMode(TRANSPARENT);
      LPCTSTR pstrEllipsis = _T("...");
      dc.DrawText(pstrEllipsis, ::lstrlen(pstrEllipsis), &lpdis->rcItem, DT_CENTER | DT_EDITCONTROL | DT_SINGLELINE | DT_VCENTER);
      return 0;
   }
};


#endif // __PROPERTYITEMEDITORS__H
