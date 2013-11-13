#ifndef __PROPERTYTREE__H
#define __PROPERTYTREE__H

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CPropertyTree - A Property Tree control
//
// Written by Bjarke Viksoe (bjarke@viksoe.dk)
// Copyright (c) 2001 Bjarke Viksoe.
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

#ifndef __ATLAPP_H__
  #error PropertyTree.h requires atlapp.h to be included first
#endif

#ifndef __ATLCTRLS_H__
  #error PropertyTree.h requires atlctrls.h to be included first
#endif

#if !((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400))
  #include <zmouse.h>
#endif //!((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400))


// Include property base class
#include "PropertyItem.h"

// Include property implementations
#include "PropertyItemEditors.h"
#include "PropertyItemImpl.h"


// Extended Tree styles
#define PTS_EX_ICONCLICK            0x00000001
#define PTS_EX_NOCOLLAPSE           0x00000002
#define PTS_EX_SINGLECLICKEDIT      0x00000004


/////////////////////////////////////////////////////////////////////////////
// The Checkmark & Option Controls

class CPropertyCheckmarkItem : public CProperty
{
public:
   CTreeViewCtrl m_tree;
   HTREEITEM m_hItem;
   bool m_bValue;

   CPropertyCheckmarkItem(LPCTSTR pstrName, bool bValue, LPARAM lParam) : 
      CProperty(pstrName, lParam),
      m_hItem(NULL), 
      m_bValue(bValue)
   {
   }

   BYTE GetKind() const 
   { 
      return PROPKIND_CHECK; 
   }

   void SetOwner(HWND hWnd, LPVOID pData)
   {
      ATLASSERT(::IsWindow(hWnd));
      ATLASSERT(m_hWndOwner==NULL); // Cannot set it twice
      m_hWndOwner = hWnd;
      m_tree.Attach( hWnd );
      m_hItem = reinterpret_cast<HTREEITEM>(pData);
      CComVariant v(m_bValue);
      SetValue(v);
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
         break;
      default:
         ATLASSERT(false);
         return FALSE;
      }

      // Update images
      int iImage = m_bValue ? 1 : 0;
      if( !IsEnabled() ) iImage += 4;
      m_tree.SetItemImage(m_hItem, iImage, iImage);
      return TRUE;
   }

   BOOL Activate(UINT action, LPARAM /*lParam*/) 
   { 
      switch( action ) {
      case PACT_SPACE:
      case PACT_CLICK:
      case PACT_DBLCLICK:
         if( IsEnabled() ) {
            CComVariant v = !m_bValue;
            ::SendMessage(m_hWndOwner, WM_USER_PROP_CHANGEDPROPERTY, (WPARAM)(VARIANT*) &v, (LPARAM) this);
         }
         break;
      }
      return TRUE;
   }
};

class CPropertyOptionItem : public CPropertyCheckmarkItem
{
public:
   CPropertyOptionItem(LPCTSTR pstrName, bool bValue, LPARAM lParam) : CPropertyCheckmarkItem(pstrName, bValue, lParam)
   {
   }

   BOOL SetValue(const VARIANT& value)
   {
      // Set value
      switch( value.vt ) {
      case VT_BOOL:
         m_bValue = (value.boolVal != VARIANT_FALSE);
         break;
      default:
         ATLASSERT(false);
         return FALSE;
      }

      // Set new image
      int iImage = m_bValue ? 3 : 2;
      if( !IsEnabled() ) iImage += 4;
      m_tree.SetItemImage(m_hItem, iImage, iImage);
      
      // If we clicked this item (rather than someone removed the checkmark)
      // we automatically exclude all siblings around us
      if( m_bValue ) {
         // NOTE: We need to use SendMessage() and a private msg, since
         //       we only have a CTreeViewCtrl and not a complete CPropertyTreeCtrl
         //       control on our hands.
         HTREEITEM hItem;
         hItem = m_tree.GetPrevSiblingItem(m_hItem);
         while( hItem != NULL ) {
            if( m_tree.SendMessage(WM_USER_PROP_SETCHECKSTATE, (WPARAM) hItem, (LPARAM) FALSE) == FALSE ) break;
            hItem = m_tree.GetPrevSiblingItem(hItem);
         }
         hItem = m_tree.GetNextSiblingItem(m_hItem);
         while( hItem != NULL ) {
            if( m_tree.SendMessage(WM_USER_PROP_SETCHECKSTATE, (WPARAM) hItem, (LPARAM) FALSE) == FALSE ) break;
            hItem = m_tree.GetNextSiblingItem(hItem);
         }
      }
      return TRUE;
   }

   BOOL Activate(UINT action, LPARAM /*lParam*/) 
   { 
      switch( action ) {
      case PACT_SPACE:
      case PACT_CLICK:
         if( IsEnabled() ) {
            CComVariant v = true;
            ::SendMessage(m_hWndOwner, WM_USER_PROP_CHANGEDPROPERTY, (WPARAM) (VARIANT*) &v, (LPARAM) this);
         }
         break;
      }
      return TRUE;
   }
};

inline HPROPERTY PropCreateCheckmark(LPCTSTR pstrName, bool bValue = false, LPARAM lParam = 0)
{
   return new CPropertyCheckmarkItem(pstrName, bValue, lParam);
}

inline HPROPERTY PropCreateOptionCheck(LPCTSTR pstrName, bool bValue = false, LPARAM lParam = 0)
{
   return new CPropertyOptionItem(pstrName, bValue, lParam);
}


/////////////////////////////////////////////////////////////////////////////
// CPropertyTree control

template< class T, class TBase = CTreeViewCtrl, class TWinTraits = CControlWinTraits >
class ATL_NO_VTABLE CPropertyTreeImpl : 
   public CWindowImpl< T, TBase, TWinTraits >,
   public CCustomDraw< T >
{
public:
   DECLARE_WND_SUPERCLASS(NULL, TBase::GetWndClassName())

   enum { HORIZ_VALUE_GAP = 4 };

   PROPERTYDRAWINFO m_di;
   CFont m_TextFont;
   CFont m_CategoryFont;
   CPen m_BorderPen;
   HWND m_hwndInplace;
   HTREEITEM m_iInplaceIndex;
   bool m_bSeenClicks;
   int m_cxColumn;

   CPropertyTreeImpl() : 
      m_hwndInplace(NULL), 
      m_iInplaceIndex(NULL), 
      m_bSeenClicks(false), 
      m_cxColumn(0)
   {
      m_di.dwExtStyle = 0;
   }

   // Operations

   BOOL SubclassWindow(HWND hWnd)
   {
      ATLASSERT(m_hWnd==NULL);
      ATLASSERT(::IsWindow(hWnd));
      BOOL bRet = CWindowImpl< T, TBase, TWinTraits >::SubclassWindow(hWnd);
      if( bRet ) _Init();
      return bRet;
   }

   void SetExtendedTreeStyle(DWORD dwExtStyle)
   {
      ATLASSERT(::IsWindow(m_hWnd));
      m_di.dwExtStyle = dwExtStyle;
      Invalidate();
   }

   DWORD GetExtendedTreeStyle() const
   {
      return m_di.dwExtStyle;
   }

   void SetColumnPosition(int iPos)
   {
      m_cxColumn = iPos;
   }

   BOOL DeleteAllItems()
   {
      ATLASSERT(::IsWindow(m_hWnd));
      _DestroyInplaceWindow();
      return TBase::DeleteAllItems();
   }

   HTREEITEM InsertItem(HPROPERTY hProp, int nImage, int nSelectedImage, HTREEITEM hParent, HTREEITEM hInsertAfter = TVI_LAST)
   {
      ATLASSERT(::IsWindow(m_hWnd));
      ATLASSERT(hProp);
      m_bSeenClicks = false;
      // NOTE: This is the only InsertItem() we support...
      UINT mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
      LPARAM lParam = (LPARAM)hProp;
      HTREEITEM hItem = TBase::InsertItem(mask, hProp->GetName(), nImage, nSelectedImage, 0, 0, lParam, hParent, hInsertAfter); 
      if( hItem ) {
         hProp->SetOwner(m_hWnd, (LPVOID) hItem);
         Expand(hParent);
      }
      return hItem;
   }

   BOOL GetItem(LPTVITEM pItem) const
   {
      ATLASSERT(::IsWindow(m_hWnd));
      // On the fly replacement of LPARAM...
      BOOL bRes = TBase::GetItem(pItem);
      if( bRes && (pItem->mask & TVIF_PARAM) != 0 ) {
         IProperty* prop = reinterpret_cast<IProperty*>(TBase::GetItemData(pItem->hItem));
         ATLASSERT(prop);
         if( prop == NULL ) return FALSE;
         pItem->lParam = prop->GetItemData();
      }
      return bRes;
   }

   BOOL SetItem(LPTVITEM pItem)
   {
      ATLASSERT(::IsWindow(m_hWnd));
      // NOTE: Cannot change these styles:
      //       TEXT is duplicated in both tree and in property class - and
      //        not really available to us.
      //       PARAM is private. Use SetItemData() instead!
      ATLASSERT((pItem->mask & (TVIF_TEXT|TVIF_PARAM))==0); 
      return TBase::SetItem(pItem);
   }

   BOOL GetItemValue(HTREEITEM hItem, VARIANT* pVal) const
   {
      ATLASSERT(::IsWindow(m_hWnd));
      IProperty* prop = reinterpret_cast<IProperty*>(TBase::GetItemData(hItem));
      ATLASSERT(prop);
      if( prop == NULL ) return FALSE;
      return prop->GetValue(pVal);
   }

   BOOL SetItemValue(HTREEITEM hItem, VARIANT* pValue)
   {
      ATLASSERT(::IsWindow(m_hWnd));
      ATLASSERT(pValue);
      IProperty* prop = reinterpret_cast<IProperty*>(TBase::GetItemData(hItem));
      ATLASSERT(prop);
      if( prop == NULL ) return FALSE;
      // Assign value and repaint
      BOOL bRes = prop->SetValue(*pValue);
      _InvalidateItem(hItem);
      // If changing selected item then recreate in-place editor
      if( m_iInplaceIndex && (GetSelectedItem() == m_iInplaceIndex) ) _SpawnInplaceWindow(prop, m_iInplaceIndex);
      return bRes;
   }

   DWORD_PTR GetItemData(HTREEITEM hItem) const
   {
      ATLASSERT(::IsWindow(m_hWnd));
      IProperty* prop = reinterpret_cast<IProperty*>(TBase::GetItemData(hItem));
      ATLASSERT(prop);
      if( prop == NULL ) return 0;
      return (DWORD_PTR) prop->GetItemData();
   }

   void SetItemData(HTREEITEM hItem, DWORD_PTR dwData)
   {
      ATLASSERT(::IsWindow(m_hWnd));
      IProperty* prop = reinterpret_cast<IProperty*>(TBase::GetItemData(hItem));
      ATLASSERT(prop);
      if( prop == NULL ) return;
      prop->SetItemData( (LPARAM) dwData );
   }

   BOOL GetCheckState(HTREEITEM hItem) const
   {
      ATLASSERT(::IsWindow(m_hWnd));
      IProperty* prop = reinterpret_cast<IProperty*>(TBase::GetItemData(hItem));
      ATLASSERT(prop);
      ATLASSERT(prop->GetKind()==PROPKIND_CHECK);
      if( prop == NULL ) return FALSE;
      if( prop->GetKind() != PROPKIND_CHECK ) return FALSE;
      CComVariant v;
      prop->GetValue(&v);
      return v.boolVal != VARIANT_FALSE;
   }

   BOOL SetCheckState(HTREEITEM hItem, BOOL bCheck)
   {
      ATLASSERT(::IsWindow(m_hWnd));
      IProperty* prop = reinterpret_cast<IProperty*>(TBase::GetItemData(hItem));
      ATLASSERT(prop);
      ATLASSERT(prop->GetKind()==PROPKIND_CHECK);
      if( prop == NULL ) return FALSE;
      if( prop->GetKind() != PROPKIND_CHECK ) return FALSE;
      // Refresh image
      CComVariant v = (bCheck == TRUE);
      if( !prop->SetValue(v) ) return FALSE;
      _InvalidateItem(hItem);
      return TRUE;
   }

   BOOL IsItemEnabled(HTREEITEM hItem) const
   {
      ATLASSERT(::IsWindow(m_hWnd));
      IProperty* prop = reinterpret_cast<IProperty*>(TBase::GetItemData(hItem));
      ATLASSERT(prop);
      if( prop == NULL ) return FALSE;
      return (DWORD_PTR) prop->IsEnabled();
   }

   void EnableItem(HTREEITEM hItem, BOOL bState)
   {
      ATLASSERT(::IsWindow(m_hWnd));
      IProperty* prop = reinterpret_cast<IProperty*>(TBase::GetItemData(hItem));
      ATLASSERT(prop);
      if( prop == NULL ) return;
      prop->SetEnabled(bState);
      // The checkmark/option tree-properties need special care, since
      // they will not repaint correctly unless changing the value.
      if( prop->GetKind() == PROPKIND_CHECK ) {
         // Refresh image hack
         CComVariant v;
         prop->GetValue(&v);
         prop->SetValue(v);
      }
      _InvalidateItem(hItem);
   }

   // Implementation

   void _Init()
   {
      ATLASSERT(::IsWindow(m_hWnd));
      ModifyStyle(TVS_CHECKBOXES | TVS_FULLROWSELECT, TVS_SHOWSELALWAYS);
      SendMessage(WM_SETTINGCHANGE);
   }

   HTREEITEM _FindProperty(HPROPERTY prop, HTREEITEM hItem = NULL) const
   {
      if( hItem == NULL ) hItem = GetRootItem();
      while( hItem != NULL ) {
         if( TBase::GetItemData(hItem) == (DWORD_PTR) prop ) return hItem;
         // Recurse into children
         HTREEITEM hChild = GetChildItem(hItem);
         if( hChild != NULL ) {
            hChild = _FindProperty(prop, hChild);
            if( hChild && (TBase::GetItemData(hChild) == (DWORD_PTR) prop) ) return hChild;
         }
         // Get next sibling
         hItem = GetNextSiblingItem(hItem);
      }
      return NULL;
   }

   void _GetInplaceRect(HTREEITEM iItem, RECT* pRect) const
   {
      ATLASSERT(iItem);
      ATLASSERT(pRect);
      RECT rcText = { 0 };
      GetItemRect(iItem, &rcText, TRUE);
      RECT rcItem = { 0 };
      GetItemRect(iItem, &rcItem, FALSE);
      ::SetRect(pRect, rcText.right + HORIZ_VALUE_GAP, rcItem.top, rcItem.right, rcItem.bottom);
      if( m_cxColumn>0 ) pRect->left = max( (int) pRect->left, m_cxColumn );
   }

   void _DestroyInplaceWindow()
   {
      if( m_hwndInplace != NULL && ::IsWindow(m_hwndInplace) ) {
         // Find property
         IProperty* prop = reinterpret_cast<IProperty*>(TBase::GetItemData(m_iInplaceIndex));
         ATLASSERT(prop);
         BYTE bKind = prop->GetKind();
         // Set focus back to our control
         if( ::GetFocus() != m_hWnd && IsChild(::GetFocus()) ) SetFocus();
         // Destroy control
         switch( bKind ) {
         case PROPKIND_CONTROL:
            ::DestroyWindow(m_hwndInplace);
            break;
         default:
            ::PostMessage(m_hwndInplace, WM_CLOSE, 0, 0L);
            break;
         }
      }
      m_hwndInplace = NULL;
      m_iInplaceIndex = NULL;
   }

   BOOL _SpawnInplaceWindow(IProperty* prop, HTREEITEM hItem)
   {
      ATLASSERT(prop);
      ATLASSERT(hItem);
      // Destroy old editor
      _DestroyInplaceWindow();
      // Do we need an editor here?
      if( hItem == NULL || hItem != GetSelectedItem() ) return FALSE;
      if( !prop->IsEnabled() ) return FALSE;
      // Create a new editor window
      RECT rcValue = { 0 };
      _GetInplaceRect(hItem, &rcValue);
      ::InflateRect(&rcValue, 0, -1);
      m_hwndInplace = prop->CreateInplaceControl(m_hWnd, rcValue);
      if( m_hwndInplace != NULL ) {
         // Activate the new editor window
         ATLASSERT(::IsWindow(m_hwndInplace));
         ::SetWindowPos(m_hwndInplace, HWND_TOP, 0,0,0,0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
         m_iInplaceIndex = hItem;
      }
      return m_hwndInplace != NULL;
   }

   void _InvalidateItem(HTREEITEM hItem)
   {
      if( hItem == NULL ) return;
      RECT rc = { 0 };
      GetItemRect(hItem, &rc, FALSE);
      InvalidateRect(&rc);
   }

   // Message map and handlers
   
   BEGIN_MSG_MAP(CPropertyTreeImpl)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
      MESSAGE_HANDLER(WM_SIZE, OnScroll)
      MESSAGE_HANDLER(WM_VSCROLL, OnScroll)
      MESSAGE_HANDLER(WM_HSCROLL, OnScroll)
      MESSAGE_HANDLER(WM_MOUSEWHEEL, OnScroll)      
      MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
      MESSAGE_HANDLER(WM_CHAR, OnChar)
      MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
      MESSAGE_HANDLER(WM_USER_PROP_NAVIGATE, OnNavigate);
      MESSAGE_HANDLER(WM_USER_PROP_UPDATEPROPERTY, OnUpdateProperty);
      MESSAGE_HANDLER(WM_USER_PROP_CANCELPROPERTY, OnCancelProperty);
      MESSAGE_HANDLER(WM_USER_PROP_CHANGEDPROPERTY, OnChangedProperty);
      MESSAGE_HANDLER(WM_USER_PROP_SETCHECKSTATE, OnSetCheckState);
      REFLECTED_NOTIFY_CODE_HANDLER(TVN_SELCHANGED, OnSelChanged)
      REFLECTED_NOTIFY_CODE_HANDLER(TVN_DELETEITEM, OnDeleteItem)
      REFLECTED_NOTIFY_CODE_HANDLER(TVN_ITEMEXPANDING, OnExpanding)
      REFLECTED_NOTIFY_CODE_HANDLER(NM_CLICK, OnClick)
      REFLECTED_NOTIFY_CODE_HANDLER(NM_DBLCLK, OnDblClick)
      CHAIN_MSG_MAP_ALT(CCustomDraw<T>, 1)
      DEFAULT_REFLECTION_HANDLER()
   END_MSG_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      LRESULT lRes = DefWindowProc();
      _Init();
      return lRes;
   }

   LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {
      // Make sure to delete editor and item-data memory
      // FIX: Thanks to Ilya Markevich for spotting this memory leak
      DeleteAllItems();
      bHandled = TRUE;
      return 0;
   }

   LRESULT OnSettingChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      // Standard colors
      m_di.clrText = ::GetSysColor(COLOR_WINDOWTEXT);
      m_di.clrBack = ::GetSysColor(COLOR_WINDOW);
      m_di.clrSelText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
      m_di.clrSelBack = ::GetSysColor(COLOR_HIGHLIGHT);
      m_di.clrDisabled = ::GetSysColor(COLOR_GRAYTEXT);
      // Border
      m_di.clrBorder = ::GetSysColor(COLOR_BTNFACE);
      if( !m_BorderPen.IsNull() ) m_BorderPen.DeleteObject();
      m_di.Border = m_BorderPen.CreatePen(PS_SOLID, 1, m_di.clrBorder);
      // Fonts
      if( !m_TextFont.IsNull() ) m_TextFont.DeleteObject();
      if( !m_CategoryFont.IsNull() ) m_CategoryFont.DeleteObject();
      LOGFONT lf = { 0 };
      HFONT hFont = (HFONT)::SendMessage(GetParent(), WM_GETFONT, 0, 0);
      if( hFont == NULL ) hFont = AtlGetDefaultGuiFont();
      ::GetObject(hFont, sizeof(lf), &lf);
      m_di.TextFont = m_TextFont.CreateFontIndirect(&lf);
      SetFont(m_di.TextFont);
      m_di.CategoryFont = m_CategoryFont.CreateFontIndirect(&lf);
      // Text metrics
      CClientDC dc(m_hWnd);
      HFONT hOldFont = dc.SelectFont(m_di.TextFont);
      dc.GetTextMetrics(&m_di.tmText);
      dc.SelectFont(hOldFont);
      // Repaint
      Invalidate();
      return 0;
   }

   LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      LRESULT lRes = DefWindowProc();
      // Spawn editor right away for selected item
      IProperty* prop = NULL;
      HTREEITEM hItem = GetSelectedItem();
      if( hItem != NULL ) {
         prop = reinterpret_cast<IProperty*>(TBase::GetItemData(hItem));
         ATLASSERT(prop);
         if( prop->IsEnabled() ) {
            prop->Activate(PACT_ACTIVATE, 0);
            _SpawnInplaceWindow(prop, hItem);
         }
      }
      return lRes;
   }

   LRESULT OnScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {      
      // HACK: When scrolling we need to destroy the in-place editor
      //       because the cursor would become out-of-sync...
      _DestroyInplaceWindow();
      bHandled = FALSE;
      return 0;
   }

   LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
   {
      bHandled = FALSE;
      HTREEITEM hItem = GetSelectedItem();
      if( hItem == NULL ) return 0;
      IProperty* prop = reinterpret_cast<IProperty*>(TBase::GetItemData(hItem));
      ATLASSERT(prop);
      switch( LOWORD(wParam) ) {
      case VK_TAB:
         prop->Activate(PACT_TAB, 0);
         break;
      case VK_F2:
      case VK_RIGHT:
      case VK_SPACE:
         _SpawnInplaceWindow(prop, hItem);
         prop->Activate(PACT_SPACE, 0);
         break;
      }
      return 0;
   }

   LRESULT OnChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
   {
      switch( wParam ) {
      case _T(' '):
         // Don't want to hear nasty Windows beep sound...
         return 0;
      default:
         bHandled = FALSE;
         return 0;
      }
   }

   LRESULT OnNavigate(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      switch( wParam) {
      case VK_UP:
      case VK_DOWN:
         SelectItem(GetNextItem(GetSelectedItem(), wParam == VK_UP ? TVGN_PREVIOUSVISIBLE : TVGN_NEXTVISIBLE));
         break;
      }
      return 0;
   }

   LRESULT OnSetCheckState(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
   {
      return SetCheckState( (HTREEITEM) wParam, (BOOL) lParam );
   }

   LRESULT OnUpdateProperty(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
   {
      // Updates a property value using an active editor window.
      // The editor window uses this message to update the attached property class.
      // Parameters:
      //   LPARAM = Window (HWND)
      HWND hWnd = reinterpret_cast<HWND>(lParam);     
      ATLASSERT(::IsWindow(hWnd));
      if( !::IsWindow(hWnd) || m_iInplaceIndex == NULL ) return 0;
      IProperty* prop = reinterpret_cast<IProperty*>(TBase::GetItemData(m_iInplaceIndex));
      if( prop == NULL ) return 0;
      // Ask owner about change
      NMPROPERTYITEM nmh = { m_hWnd, GetDlgCtrlID(), PIN_ITEMCHANGING, prop };
      if( ::SendMessage(GetParent(), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh) == 0 ) {
         if( !prop->SetValue(hWnd) ) {
            ::MessageBeep((UINT)-1);
            return 0;
         }
         // Let owner know
         nmh.hdr.code = PIN_ITEMCHANGED;
         ::SendMessage(GetParent(), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh);
         // Repaint item
         _InvalidateItem(m_iInplaceIndex);
         // Destroy in-place control...
         _DestroyInplaceWindow();
      }
      return 0;
   }

   LRESULT OnCancelProperty(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
   {
      // Updates a property value using an active editor window.
      // The editor window uses this message to update the attached property class.
      // Parameters:
      //   LPARAM = Window (HWND)
      HWND hWnd = reinterpret_cast<HWND>(lParam);     
      ATLASSERT(::IsWindow(hWnd));
      if( !::IsWindow(hWnd) || m_iInplaceIndex == NULL ) return 0;
      IProperty* prop = reinterpret_cast<IProperty*>(TBase::GetItemData(m_iInplaceIndex));
      ATLASSERT(prop);
      // Repaint item
      _InvalidateItem(m_iInplaceIndex);
      // Recycle in-place control so it displays the (old) value
      HTREEITEM hItem = _FindProperty(prop);
      if( hItem != NULL && (hItem == m_iInplaceIndex) ) _SpawnInplaceWindow(prop, hItem);
      return 0;
   }

   LRESULT OnChangedProperty(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
   {
      // Updates a property value.
      // A property class uses this message to make sure the corresponding editor window
      // is updated as well.
      // Parameters:
      //   WPARAM = New value (VARIANT*)
      //   LPARAM = Property (IProperty*)
      IProperty* prop = reinterpret_cast<IProperty*>(lParam);
      VARIANT* pVariant = reinterpret_cast<VARIANT*>(wParam);
      ATLASSERT(prop && pVariant);
      if( prop == NULL || pVariant == NULL ) return 0;
      // Ask owner about change
      NMPROPERTYITEM nmh = { m_hWnd, GetDlgCtrlID(), PIN_ITEMCHANGING, prop };
      if( ::SendMessage(GetParent(), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh) == 0 ) {
         // Set new value
         // NOTE: Do not call this from IProperty::SetValue(VARIANT*) = endless loop
         if( !prop->SetValue(*pVariant) ) ::MessageBeep((UINT)-1);
         // Let owner know
         nmh.hdr.code = PIN_ITEMCHANGED;
         ::SendMessage(GetParent(), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh);
      }
      // Locate the updated tree item
      HTREEITEM hItem = _FindProperty(prop);
      // Repaint item
      _InvalidateItem(hItem);
      // Recycle in-place control so it displays the new value
      if( hItem != NULL && (hItem == m_iInplaceIndex) ) _SpawnInplaceWindow(prop, hItem);
      return 0;
   }

   LRESULT OnDeleteItem(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
   {
      LPNMTREEVIEW pnmtv = (LPNMTREEVIEW) pnmh;
      _DestroyInplaceWindow();
      ATLASSERT(pnmtv->itemOld.lParam);
      delete reinterpret_cast<IProperty*>(pnmtv->itemOld.lParam);
      return 0;
   }

   LRESULT OnClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
   {
      m_bSeenClicks = true;

      // Get the item we're clicking on
      DWORD dwPos = ::GetMessagePos();
      POINT pt = { GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos) };
      ScreenToClient(&pt);
      UINT uFlags = 0;
      HTREEITEM hItem = HitTest(pt, &uFlags);
      // Send click event
      if( hItem != NULL ) {
         IProperty* prop = reinterpret_cast<IProperty*>(TBase::GetItemData(hItem));
         ATLASSERT(prop);
         // Test for icon-area click restriction
         bool bClick = (prop->IsEnabled() == TRUE);
         switch( prop->GetKind() ) {
         case PROPKIND_CHECK:
            bClick = ((m_di.dwExtStyle & PTS_EX_ICONCLICK) == 0) || ((uFlags & TVHT_ONITEMICON) != 0);
            if( (uFlags & TVHT_ONITEMRIGHT) != 0 ) bClick = false;
            break;
         case PROPKIND_SIMPLE:
            if( (uFlags & TVHT_ONITEMRIGHT) != 0 ) bClick = false;
            break;
         default:
            if( GetSelectedItem() != hItem ) SelectItem(hItem); // When clicking on the right
            if( (uFlags & TVHT_ONITEMRIGHT) != 0 ) bClick = true;
            break;
         }
         if( bClick ) {
            // Ask owner first
            NMPROPERTYITEM nmh = { m_hWnd, GetDlgCtrlID(), PIN_CLICK, prop };
            if( ::SendMessage(GetParent(), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh) == 0 ) {
               // Send Click action
               LPARAM lParam = ::GetMessagePos();
               prop->Activate(PACT_CLICK, lParam);
            }
         }
      }

      bHandled = FALSE;
      return 0;
   }

   LRESULT OnDblClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
   {
      HTREEITEM hItem = GetSelectedItem();
      if( hItem != NULL ) {
         IProperty* prop = reinterpret_cast<IProperty*>(TBase::GetItemData(hItem));
         ATLASSERT(prop);
         // Ask owner first
         NMPROPERTYITEM nmh = { m_hWnd, GetDlgCtrlID(), PIN_DBLCLICK, prop };
         if( ::SendMessage(GetParent(), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh) == 0 ) {
            // Send DblClick action
            LPARAM lParam = ::GetMessagePos();
            prop->Activate(PACT_DBLCLICK, lParam);
         }
      }
      bHandled = FALSE;
      return 0;
   }

   LRESULT OnSelChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
   {
      // No editor, please
      _DestroyInplaceWindow();
      IProperty* prop = NULL;
      // Only items with selection...
      HTREEITEM hItem = GetSelectedItem();
      if( hItem != NULL ) {
         prop = reinterpret_cast<IProperty*>(TBase::GetItemData(hItem));
         ATLASSERT(prop);
         if( prop->IsEnabled() ) {
            _SpawnInplaceWindow(prop, hItem);
            prop->Activate(PACT_ACTIVATE, 0);
            if( (m_di.dwExtStyle & PTS_EX_SINGLECLICKEDIT) != 0 ) {
               if( prop->GetKind() == PROPKIND_EDIT ) prop->Activate(PACT_DBLCLICK, 0);
            }
         }
      }
      // Let owner know
      NMPROPERTYITEM nmh = { m_hWnd, GetDlgCtrlID(), PIN_SELCHANGED, prop };
      ::SendMessage(GetParent(), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh);
      return 0;
   }

   LRESULT OnExpanding(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
   {
      // Test for style that prevents collapsing
      if( (m_di.dwExtStyle & PTS_EX_NOCOLLAPSE) != 0 && m_bSeenClicks ) return TRUE;

      // Convert to PropertyTree notifications...
      LPNMTREEVIEW nmtv = (LPNMTREEVIEW) pnmh;
      HTREEITEM hItem = nmtv->itemNew.hItem;
      IProperty* prop = reinterpret_cast<IProperty*>(TBase::GetItemData(hItem));
      ATLASSERT(prop);
      UINT nMsg = nmtv->action == TVE_COLLAPSE ? PIN_COLLAPSING : PIN_EXPANDING;
      NMPROPERTYITEM nmh = { m_hWnd, GetDlgCtrlID(), nMsg, prop };
      if( ::SendMessage(GetParent(), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh) != 0 ) return TRUE;
      
      // Let others have a go...
      bHandled = FALSE;
      return 0;
   }

   // Custom painting

   DWORD OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
   {
      return CDRF_NOTIFYITEMDRAW;   // We need per-item notifications
   }

   DWORD OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
   {
      return CDRF_NOTIFYPOSTPAINT;   // We need more notifications
   }

   DWORD OnItemPostPaint(int /*idCtrl*/, LPNMCUSTOMDRAW lpNMCustomDraw)
   {
      T* pT = static_cast<T*>(this);
      pT->DrawTreeItem(lpNMCustomDraw->hdc, (HTREEITEM) lpNMCustomDraw->dwItemSpec, lpNMCustomDraw->uItemState);
      return CDRF_DODEFAULT;
   }

   // Overridables

   void DrawTreeItem(CDCHandle dc, HTREEITEM hItem, UINT iState)
   {
      IProperty* prop = reinterpret_cast<IProperty*>(TBase::GetItemData(hItem));
      ATLASSERT(prop);

      // Get paint rectangle
      RECT rcItem = { 0 };
      _GetInplaceRect(hItem, &rcItem);

      // Setup drawinfo
      PROPERTYDRAWINFO di = m_di;
      di.hDC = dc;
      di.rcItem = rcItem;
      di.state = 0;
      if( !prop->IsEnabled() ) di.state |= ODS_DISABLED;
      if( hItem == m_iInplaceIndex ) di.state |= ODS_COMBOBOXEDIT;

      // A disabled TreeView control is painted grey in Windows2000
      if( (GetStyle() & WS_DISABLED) != 0 ) {
         di.clrText = di.clrDisabled;
         di.clrBack = ::GetSysColor(COLOR_BTNFACE);
      }

      // Erase background
      dc.FillSolidRect(&rcItem, di.clrBack);

      // Draw border
      bool bDrawBorder = false;
      switch( prop->GetKind() ) {
      case PROPKIND_SIMPLE:
      case PROPKIND_CHECK:
         bDrawBorder = false;
         break;
      default:
         bDrawBorder = ((iState & CDIS_SELECTED) != 0);
         break;
      }
      if( bDrawBorder ) {
         RECT rcBorder = rcItem;
         ::OffsetRect(&rcBorder, -1, 0);
         CBrush brBorder;
         brBorder.CreateSolidBrush(di.clrBorder);
         dc.FrameRect(&rcBorder, brBorder);
      }

      // Paint value
      HFONT hOldFont = dc.SelectFont(di.TextFont);
      prop->DrawValue(di);
      dc.SelectFont(hOldFont);
  }
};

class CPropertyTreeCtrl : public CPropertyTreeImpl<CPropertyTreeCtrl>
{
public:
   DECLARE_WND_SUPERCLASS(_T("WTL_PropertyTree"), GetWndClassName())
};


#endif // __PROPERTYTREE__H

