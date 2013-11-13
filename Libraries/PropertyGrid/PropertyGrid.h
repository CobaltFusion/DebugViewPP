#ifndef __PROPERTYGRID__H
#define __PROPERTYGRID__H

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CPropertyGrid - A simple grid control
//
// Written by Bjarke Viksoe (bjarke@viksoe.dk)
// Copyright (c) 2002-2003 Bjarke Viksoe.
// Thanks to Ludvig A. Norin for the PGS_EX_ADDITEMATEND feature.
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
  #error PropertyGrid.h requires atlapp.h to be included first
#endif

#if (_WTL_VER < 0x0700)
   #error This file requires WTL version 7.0 or higher
#endif

#ifndef __ATLCTRLS_H__
  #error PropertyGrid.h requires atlctrls.h to be included first
#endif

#if (_WIN32_IE < 0x0400)
  #error PropertyGrid.h requires IE4
#endif

#if !((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400))
  #include <zmouse.h>
#endif //!((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400))


// Include property base class
#include "PropertyItem.h"

// Include property implementations
#include "PropertyItemEditors.h"
#include "PropertyItemImpl.h"



/////////////////////////////////////////////////////////////////////////////
// CPropertyGrid control

// Extended Grid styles
#define PGS_EX_SINGLECLICKEDIT   0x00000001
#define PGS_EX_NOGRID            0x00000002
#define PGS_EX_TABNAVIGATION     0x00000004
#define PGS_EX_NOSHEETNAVIGATION 0x00000008
#define PGS_EX_FULLROWSELECT     0x00000010
#define PGS_EX_INVERTSELECTION   0x00000020
#define PGS_EX_ADDITEMATEND      0x00000040



/////////////////////////////////////////////////////////////////////////////
// The "AppendAction" property (PGS_EX_ADDITEMATEND support)

class CPropertyAppendActionItem : public CProperty
{
public:
   CPropertyAppendActionItem(LPCTSTR pstrName, LPARAM lParam) : 
      CProperty(pstrName, lParam)
   {
   }

   BYTE GetKind() const 
   { 
      return PROPKIND_SIMPLE;
   }

   void DrawValue(PROPERTYDRAWINFO& di)
   {
#ifdef IDS_LASTVALUE
      TCHAR szText[128] = { 0 };
      ::LoadString(_Module.GetResourceInstance(), IDS_LASTVALUE, szText, (sizeof(szText)/sizeof(TCHAR))-1);
      LPCTSTR pstrText = szText;
#else
      LPCTSTR pstrText = _T("<< Click here to add a new item >>");
#endif  // IDS_LASTVALUE
      CDCHandle dc(di.hDC);
      dc.SetBkMode(TRANSPARENT);
      dc.SetTextColor((di.state & ODS_DISABLED) != 0 ? di.clrDisabled : di.clrText);
      dc.SetBkColor(di.clrBack);
      RECT rcText = di.rcItem;
      rcText.left += PROP_TEXT_INDENT;
      dc.DrawText(pstrText, -1, 
         &rcText, 
         DT_LEFT | DT_SINGLELINE | DT_EDITCONTROL | DT_NOPREFIX | DT_END_ELLIPSIS | DT_VCENTER);
   }

   BOOL Activate(UINT action, LPARAM /*lParam*/) 
   { 
      switch( action ) {
      case PACT_SPACE:
      case PACT_CLICK:
      case PACT_DBLCLICK:
         // Send AddItem notification to parent of control
         NMPROPERTYITEM nmh = { m_hWndOwner, ::GetDlgCtrlID(m_hWndOwner), PIN_ADDITEM, NULL };
         ::SendMessage(::GetParent(m_hWndOwner), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh);
         break;
      }
      return TRUE;
   }
};

inline HPROPERTY PropCreateAppendActionItem(LPCTSTR pstrName, LPARAM lParam = 0)
{
   return new CPropertyAppendActionItem(pstrName, lParam);
}


/////////////////////////////////////////////////////////////////////////////
// Block property - looks like a header (or button)

class CPropertyBlockItem : public CProperty
{
public:
   CPropertyBlockItem(LPCTSTR pstrName, LPARAM lParam) : 
      CProperty(pstrName, lParam)
   {
   }

   BYTE GetKind() const 
   { 
      return PROPKIND_SIMPLE;
   }

   void DrawValue(PROPERTYDRAWINFO& di)
   {
      RECT rc = di.rcItem;
      rc.bottom--;
      ::DrawFrameControl(di.hDC, &rc, DFC_BUTTON, DFCS_BUTTONPUSH);
   }

   BOOL Activate(UINT action, LPARAM /*lParam*/) 
   { 
      switch( action ) {
      case PACT_SPACE:
      case PACT_CLICK:
      case PACT_DBLCLICK:
         // Send AddItem notification to parent of control
         NMPROPERTYITEM nmh = { m_hWndOwner, ::GetDlgCtrlID(m_hWndOwner), PIN_BROWSE, NULL };
         ::SendMessage(::GetParent(m_hWndOwner), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh);
         break;
      }
      return TRUE;
   }
};

inline HPROPERTY PropCreateBlockItem(LPCTSTR pstrName, LPARAM lParam = 0)
{
   return new CPropertyBlockItem(pstrName, lParam);
}


/////////////////////////////////////////////////////////////////////////////
// The Property Grid control

template< class T, class TBase = CListViewCtrl, class TWinTraits = CWinTraitsOR<LVS_SINGLESEL|LVS_SHOWSELALWAYS> >
class ATL_NO_VTABLE CPropertyGridImpl : 
   public CWindowImpl< T, TBase, TWinTraits >,
   public CCustomDraw< T >
{
public:
   DECLARE_WND_SUPERCLASS(NULL, TBase::GetWndClassName())

   CHeaderCtrl m_ctrlHeader;
   PROPERTYDRAWINFO m_di;
   CFont m_TextFont;
   CFont m_CategoryFont;
   CPen m_BorderPen;
   HWND m_hwndInplace;
   int m_iInplaceRow;
   int m_iInplaceCol;
   int m_nColumns;
   int m_iSelectedRow;
   int m_iSelectedCol;

   CPropertyGridImpl() : 
      m_hwndInplace(NULL), 
      m_iInplaceRow(-1), 
      m_iInplaceCol(-1), 
      m_nColumns(0), 
      m_iSelectedRow(-1), 
      m_iSelectedCol(-1)
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

   void SetExtendedGridStyle(DWORD dwExtStyle)
   {
      // Handle change of PGS_EX_ADDITEMATEND flag
      if( (m_di.dwExtStyle & PGS_EX_ADDITEMATEND) != 0 
          && (dwExtStyle & PGS_EX_ADDITEMATEND) == 0 ) 
      {
         // Remove AppendAction item
         DeleteItem(TBase::GetItemCount()-1);
      } 
      if( (dwExtStyle & PGS_EX_ADDITEMATEND) != 0 
          && (m_di.dwExtStyle & PGS_EX_ADDITEMATEND) == 0 ) 
      {
         // Add AppendAction item
         InsertItem(TBase::GetItemCount(), PropCreateAppendActionItem(_T(""))); 
      }
      // Assign new style
      m_di.dwExtStyle = dwExtStyle;
      // Recalc colours and fonts
      SendMessage(WM_SETTINGCHANGE);
   }

   DWORD GetExtendedGridStyle() const
   {
      return m_di.dwExtStyle;
   }

   BOOL SelectItem(int iRow, int iCol = 0)
   {
      // No editor and remove focus
      _DestroyInplaceWindow();
      _InvalidateItem(m_iSelectedRow, m_iSelectedCol);
      // Select new item. If on same row then use internal update.
      m_iSelectedCol = iCol;
      if( GetSelectedIndex() == m_iSelectedRow && m_iSelectedRow == iRow ) {         
         NMLISTVIEW nmlv = { m_hWnd, 0, 0, m_iSelectedRow, m_iSelectedCol, LVIS_SELECTED };
         BOOL bDummy = FALSE;
         OnSelChanged(0, reinterpret_cast<LPNMHDR>(&nmlv), bDummy);
         return TRUE;
      }
      else {
         return TBase::SelectItem(iRow);
      }
   }

   int GetItemCount() const
   {
      if( (m_di.dwExtStyle & PGS_EX_ADDITEMATEND) != 0 ) return max(0, TBase::GetItemCount() - 1);
      return TBase::GetItemCount();
   }

   int InsertItem(int nItem, HPROPERTY hProp)
   {
      // NOTE: This is the only InsertItem() we support...
      ATLASSERT(::IsWindow(m_hWnd));
      ATLASSERT(hProp);
      // You must have initialized columns before calling this!
      // And you are not allowed to add columns once the list is populated!
      if( m_nColumns == 0 ) m_nColumns = m_ctrlHeader.GetItemCount();
      ATLASSERT(m_nColumns>0);
      ATLASSERT(m_ctrlHeader.GetItemCount()==m_nColumns);
      // Create a place-holder for all sub-items
      IProperty** props = NULL;
      ATLTRY( props = new IProperty*[m_nColumns] );
      ATLASSERT(props);
      if( props == NULL ) return -1;
      ::ZeroMemory(props, sizeof(IProperty*) * m_nColumns);
      props[0] = hProp;
      // Finally create the listview item itself...
      if( nItem < 0 || nItem > GetItemCount() ) nItem = GetItemCount();
      UINT mask = LVIF_TEXT | LVIF_PARAM;
      int iItem = TBase::InsertItem(mask, nItem, hProp->GetName(), 0, 0, 0, (LPARAM) props); 
      if( iItem != -1 ) hProp->SetOwner(m_hWnd, NULL);
      return iItem;
   }

   BOOL SetSubItem(int nItem, int nSubItem, HPROPERTY hProp)
   {
      ATLASSERT(::IsWindow(m_hWnd));
      ATLASSERT(hProp);
      ATLASSERT(nSubItem>=0);
      ATLASSERT(nSubItem<m_nColumns);
      IProperty** props = reinterpret_cast<IProperty**>(TBase::GetItemData(nItem));
      ATLASSERT(props);
      ATLASSERT(props[nSubItem]==NULL); // Do not replace HPROPERTY nodes.
      if( props == NULL ) return FALSE;
      if( nSubItem < 0 || nSubItem >= m_nColumns ) return FALSE;
      props[nSubItem] = hProp;
      hProp->SetOwner(m_hWnd, NULL);
      // Trick ListView into thinking there is a subitem...
      return TBase::SetItemText(nItem, nSubItem, _T(""));
   }

   BOOL GetItemText(int iItem, int iSubItem, LPTSTR pstrText, UINT cchMax) const
   {
      return GetItemText(GetProperty(iItem, iSubItem), pstrText, cchMax);
   }

   BOOL GetItemText(HPROPERTY hProp, LPTSTR pstrText, UINT cchMax) const
   {
      ATLASSERT(::IsWindow(m_hWnd));
      ATLASSERT(hProp);
      ATLASSERT(!::IsBadWritePtr(pstrText,cchMax));
      if( hProp == NULL || pstrText == NULL ) return FALSE;
      return hProp->GetDisplayValue(pstrText, cchMax);
   }

   BOOL GetItemValue(HPROPERTY hProp, VARIANT* pValue) const
   {
      ATLASSERT(::IsWindow(m_hWnd));
      ATLASSERT(hProp);
      ATLASSERT(pValue);
      if( hProp == NULL || pValue == NULL ) return FALSE;
      return hProp->GetValue(pValue);
   }

   BOOL SetItemValue(HPROPERTY hProp, VARIANT* pValue)
   {
      ATLASSERT(::IsWindow(m_hWnd));
      ATLASSERT(hProp);
      ATLASSERT(pValue);
      if( hProp == NULL || pValue == NULL ) return FALSE;
      // Assign value and repaint
      BOOL bRes = hProp->SetValue(*pValue);
      // Find property position and repaint it...
      int nItem = -1, nSubItem = -1;
      if( !FindProperty(hProp, nItem, nSubItem) ) return FALSE;
      _InvalidateItem(nItem, nSubItem);
      // If changing selected item then recreate in-place editor
      if( (m_hwndInplace != NULL) && (nItem == m_iInplaceRow) && (nSubItem == m_iInplaceCol) ) _SpawnInplaceWindow(hProp, m_iInplaceRow, m_iInplaceCol);
      return bRes;
   }

   int GetSelectedColumn() const
   {
      ATLASSERT(::IsWindow(m_hWnd));
      return m_iSelectedCol;
   }

   BOOL DeleteColumn(int nCol)
   {
      ATLASSERT(::IsWindow(m_hWnd));
      if( TBase::GetItemCount() == 0 ) {
         m_nColumns = 0;
         return TBase::DeleteColumn(nCol);
      }
      ATLASSERT(false); // Remove items first
      return FALSE;
   }

   BOOL GetColumnCount() const
   {
      ATLASSERT(::IsWindow(m_hWnd));
      return GetHeader().GetItemCount();
   }

   BOOL FindProperty(IProperty* prop, int& iItem, int& iSubItem) const
   {
      // Looks up the item index based on the property class.
      // The property class may be a subitem, so we need to scan ALL properties.
      ATLASSERT(prop);
      // Hmm, this would be an awfully slooow method!
      // So most of the time we're really searching for the in-place editor...
      if( m_iInplaceRow != -1 ) {
         IProperty** props = reinterpret_cast<IProperty**>(TBase::GetItemData(m_iInplaceRow));
         if( props != NULL ) {
            for( int i = 0; i < m_nColumns; i++ ) {
               if( props[i] == prop ) {
                  iItem = m_iInplaceRow;
                  iSubItem = i;
                  return TRUE;
               }
            }
         }
      }
      // Ok, scan all items...
      iItem = GetNextItem(-1, LVNI_ALL);
      while( iItem != -1 ) {
         IProperty** props = reinterpret_cast<IProperty**>(TBase::GetItemData(iItem));
         for( int i = 0; i < m_nColumns; i++ ) {
            if( props[i] == prop ) {
               iSubItem = i;
               return TRUE;
            }
         }
         iItem = GetNextItem(iItem, LVNI_ALL);
      }
      return FALSE;
   }

   IProperty* GetProperty(int iRow, int iCol) const
   {
      ATLASSERT(iCol >= 0 && iCol < m_nColumns);
      ATLASSERT(iRow >= 0 && iRow < TBase::GetItemCount());
      if( iCol < 0 || iCol >= m_nColumns ) return NULL;
      if( iRow < 0 || iRow >= TBase::GetItemCount() ) return NULL;
      IProperty** props = reinterpret_cast<IProperty**>(TBase::GetItemData(iRow));
      ATLASSERT(props);
      if( props == NULL ) return NULL;
      IProperty* prop = props[iCol];
      ATLASSERT(prop); // If you hit this assert, most likely you forgot to add properties to all subitems.
                       // Use read-only properties to fill out with dummies.
      return prop;
   }

   LPARAM GetItemData(HPROPERTY hProp) const
   {
      ATLASSERT(::IsWindow(m_hWnd));
      ATLASSERT(hProp);
      if( hProp == NULL ) return 0;
      return hProp->GetItemData();
   }

   void SetItemData(HPROPERTY hProp, LPARAM lParam)
   {
      ATLASSERT(::IsWindow(m_hWnd));
      ATLASSERT(hProp);
      if( hProp == NULL ) return;
      hProp->SetItemData(lParam);
   }

   BOOL GetItemEnabled(HPROPERTY hProp) const
   {
      ATLASSERT(hProp);
      if( hProp == NULL ) return FALSE;
      return hProp->IsEnabled();
   }

   void SetItemEnabled(HPROPERTY hProp, BOOL bEnable)
   {
      ATLASSERT(::IsWindow(m_hWnd));
      ATLASSERT(hProp);
      if( hProp == NULL ) return;
      hProp->SetEnabled(bEnable);
      // Repaint item...
      int nItem = -1, nSubItem = -1;
      if( !FindProperty(hProp, nItem, nSubItem) ) return;
      _InvalidateItem(nItem, nSubItem);
   }

   void Navigate(UINT wCode)
   {
      SendMessage(WM_USER_PROP_NAVIGATE, wCode);
   }

   // Unsupported methods

   BOOL SetItem(int /*nItem*/, int /*nSubItem*/, UINT /*nMask*/, LPCTSTR /*lpszItem*/,
      int /*nImage*/, UINT /*nState*/, UINT /*nStateMask*/, LPARAM /*lParam*/)
   {
      ATLASSERT(false);
      return FALSE;
   }

   BOOL SetItemText(int /*nItem*/, int /*nSubItem*/, LPCTSTR /*lpszText*/)
   {
      ATLASSERT(false);
      return FALSE;
   }

   DWORD SetViewType(DWORD /*dwType*/)
   {
      ATLASSERT(false);
      return FALSE;
   }

   CEdit EditLabel(int /*nItem*/)
   {
      ATLASSERT(false);
      return NULL;
   }

   // Implementation

   void _Init()
   {
      ATLASSERT(::IsWindow(m_hWnd));
      // We need the LVS_SINGLESEL style
      ATLASSERT((GetStyle() & (LVS_SINGLESEL))==(LVS_SINGLESEL));
      ATLASSERT((GetStyle() & (LVS_EDITLABELS|LVS_OWNERDRAWFIXED|LVS_OWNERDATA))==0);
      // Prepare ListView control
      TBase::SetViewType(LVS_REPORT);
      SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);
      m_ctrlHeader = GetHeader();
      // Update colours and text
      SendMessage(WM_SETTINGCHANGE);
   }

   BOOL _SpawnInplaceWindow(IProperty* prop, int iItem, int iSubItem)
   {
      ATLASSERT(prop);
      ATLASSERT(iItem!=-1);
      ATLASSERT(iSubItem!=-1);
      // Destroy old in-place editor
      _DestroyInplaceWindow();
      // Do we need an editor here?
      if( iItem == -1 || iSubItem == -1 || iItem != GetSelectedIndex() ) return FALSE;
      if( !prop->IsEnabled() ) return FALSE;
      // Create a new editor window
      RECT rcValue = { 0 };
      _GetSubItemRect(iItem, iSubItem, &rcValue);
      rcValue.right--;   // Let the borders
      rcValue.bottom--;  // ... display (right/bottom)
      m_hwndInplace = prop->CreateInplaceControl(m_hWnd, rcValue);
      if( m_hwndInplace ) {
         // Activate the new editor window
         ATLASSERT(::IsWindow(m_hwndInplace));
         ::SetWindowPos(m_hwndInplace, HWND_TOP, 0,0,0,0, SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE);
         ::SetFocus(m_hwndInplace);
         m_iInplaceRow = iItem;
         m_iInplaceCol = iSubItem;
         return TRUE;
      }
      return m_hwndInplace != NULL;
   }

   void _DestroyInplaceWindow()
   {
      if( ::IsWindow(m_hwndInplace) ) {
         // Find property
         IProperty* prop = GetProperty(m_iInplaceRow, m_iInplaceCol);
         ATLASSERT(prop);
         BYTE bKind = prop->GetKind();
         // Remember who the inplace was before we proceed
         HWND hwndInplace = m_hwndInplace;
         m_hwndInplace = NULL;
         m_iInplaceRow = -1;
         m_iInplaceCol = -1;
         // Set focus back to our control
         // This could cause a new call to this method, so this is
         // why we cached stuff above.
         if( ::GetFocus() != m_hWnd && IsChild(::GetFocus()) ) SetFocus();
         // Destroy control
         switch( bKind ) {
         case PROPKIND_CONTROL:
            ::DestroyWindow(hwndInplace);
            break;
         default:
            ::PostMessage(hwndInplace, WM_CLOSE, 0, 0L);
            break;
         }
      }
      else {
         m_hwndInplace = NULL;
         m_iInplaceRow = -1;
         m_iInplaceCol = -1;
      }
   }

   void _GetSubItemRect(int iItem, int iSubItem, RECT* pRect) const
   {
      if( iSubItem == 0 && _IsAppendActionItem(iItem) ) {
         GetItemRect(iItem, pRect, LVIR_BOUNDS);
      }
      else if( iSubItem == 0 ) {
         GetItemRect(iItem, pRect, LVIR_BOUNDS);
         if( m_nColumns > 1 ) {
            RECT rcSecondColumn = { 0 };
            GetSubItemRect(iItem, 1, LVIR_BOUNDS, &rcSecondColumn);
            pRect->right = rcSecondColumn.left - 1;
         }
      }
      else {
         GetSubItemRect(iItem, iSubItem, LVIR_BOUNDS, pRect);
      }
   }

   void _InvalidateItem(int iItem, int iSubItem)
   {
      if( iItem == -1 || iSubItem == -1 ) return;
      RECT rc = { 0 };
      _GetSubItemRect(iItem, iSubItem, &rc);
      InvalidateRect(&rc);
   }

   bool _IsValidSelection() const
   {
      ATLASSERT(m_iSelectedRow==GetSelectedIndex());  // Should always be in sync!
      return (m_iSelectedRow != -1) && (m_iSelectedCol != -1);
   }

   bool _IsAppendActionItem(int iItem) const
   {
      return (m_di.dwExtStyle & PGS_EX_ADDITEMATEND) != 0 
             && (iItem == TBase::GetItemCount() - 1);
   }

   // Message map and handlers

   BEGIN_MSG_MAP(CPropertyGridImpl)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      MESSAGE_HANDLER(WM_SETFOCUS, OnFocus)
      MESSAGE_HANDLER(WM_KILLFOCUS, OnFocus)
      MESSAGE_HANDLER(WM_SIZE, OnScroll)
      MESSAGE_HANDLER(WM_VSCROLL, OnScroll)
      MESSAGE_HANDLER(WM_HSCROLL, OnScroll)
      MESSAGE_HANDLER(WM_MOUSEWHEEL, OnScroll)      
      MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
      MESSAGE_HANDLER(WM_CHAR, OnChar)
      MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
      MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
      MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
      MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDown)
      MESSAGE_HANDLER(WM_USER_PROP_NAVIGATE, OnNavigate);
      MESSAGE_HANDLER(WM_USER_PROP_UPDATEPROPERTY, OnUpdateProperty);
      MESSAGE_HANDLER(WM_USER_PROP_CANCELPROPERTY, OnCancelProperty);
      MESSAGE_HANDLER(WM_USER_PROP_CHANGEDPROPERTY, OnChangedProperty);
      NOTIFY_CODE_HANDLER(HDN_ITEMCHANGEDA, OnHeaderChanging)
      NOTIFY_CODE_HANDLER(HDN_ITEMCHANGEDW, OnHeaderChanging)
      NOTIFY_CODE_HANDLER(HDN_ITEMCHANGINGA, OnHeaderChanging) // See Q183258 why we all need these
      NOTIFY_CODE_HANDLER(HDN_ITEMCHANGINGW, OnHeaderChanging)
      NOTIFY_CODE_HANDLER(HDN_TRACKA, OnHeaderChanging)
      NOTIFY_CODE_HANDLER(HDN_TRACKW, OnHeaderChanging)
      NOTIFY_CODE_HANDLER(HDN_DIVIDERDBLCLICKA, OnHeaderDblClick)
      NOTIFY_CODE_HANDLER(HDN_DIVIDERDBLCLICKW, OnHeaderDblClick)
      REFLECTED_NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnSelChanged)
      REFLECTED_NOTIFY_CODE_HANDLER(LVN_DELETEITEM, OnDeleteItem)
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
      // Make sure we clean up all items...
      // FIX: Thanks to Ilya Markevich for spotting this memory leak
      DeleteAllItems();
      bHandled = TRUE;
      return 0;
   }

   LRESULT OnFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {      
      // Avoid focus-rectangle problem in ownerdrawn ListView
      _InvalidateItem(m_iSelectedRow, m_iSelectedCol);
      bHandled = FALSE;
      return 0;
   }

   LRESULT OnScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {      
      // HACK: When scrolling we need to destroy the in-place editor
      //       because the cursor would become out-of-sync...
      _DestroyInplaceWindow();
      bHandled = FALSE;
      return 0;
   }

   LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      // Repaint previous item in any case
      _InvalidateItem(m_iSelectedRow, m_iSelectedCol);

      // Gather information about clicked item.
      // We need this information before the control processes the
      // actual click event...
      LVHITTESTINFO hti = { 0 };
      DWORD dwPos = ::GetMessagePos();
      POINTSTOPOINT(hti.pt, dwPos);
      ScreenToClient(&hti.pt);
      int iItem = SubItemHitTest(&hti);
      int iOldRow = m_iSelectedRow;
      int iOldColumn = m_iSelectedCol;

      // FIX: To prevent selection from jumping to colunm 0 since
      //      the DefWindowProc() below invokes LVN_ITEMCHANGED, which
      //      in turn sets the selection to the correct row (but not column)!
      m_iSelectedCol = hti.iSubItem;

      _DestroyInplaceWindow();

      // Let control process WM_LBUTTONDOWN event!
      // This may cause LVN_ITEMCHANGED to fire...
      LRESULT lRes = DefWindowProc();

      // Check if we've changed selection.
      // We delayed this to now, because we need to check
      // if we clicked on the same item...
      if( iItem != -1 ) 
      {
         // Do click thing...
         bool bIsAppendActionItem = _IsAppendActionItem(hti.iItem);
         if( bIsAppendActionItem ) hti.iSubItem = 0;
         IProperty* prop = GetProperty(hti.iItem, hti.iSubItem);
         if( prop == NULL ) return 0;

         // Ask owner first
         NMPROPERTYITEM nmh = { m_hWnd, GetDlgCtrlID(), PIN_CLICK, prop };
         if( ::SendMessage(GetParent(), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh) == 0 ) {

            // Property is allowed to react on click event
            LPARAM lParam = ::GetMessagePos();
            prop->Activate(PACT_CLICK, lParam);

            // Set new selection...
            m_iSelectedRow = hti.iItem;
            m_iSelectedCol = hti.iSubItem;

            if( !bIsAppendActionItem ) {         
               
               // Generate selection change notification on pure horizontal moves
               if( (iOldRow == m_iSelectedRow) && (iOldColumn != m_iSelectedCol) ) {
                  // Let owner know
                  NMPROPERTYITEM nmh = { m_hWnd, GetDlgCtrlID(), PIN_SELCHANGED, prop };
                  ::SendMessage(GetParent(), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh);
               }

               // Recycle in-place editor...
               if( prop->IsEnabled() ) {
                  bool fActivate = false;
                  if( iOldRow == m_iSelectedRow && iOldColumn == m_iSelectedCol ) fActivate = true;
                  if( (m_di.dwExtStyle & PGS_EX_SINGLECLICKEDIT) != 0 ) fActivate = true;
                  if( fActivate ) {
                     if( _SpawnInplaceWindow(prop, m_iSelectedRow, m_iSelectedCol) ) {
                        prop->Activate(PACT_SPACE, 0);
                     }
                     else {
                        prop->Activate(PACT_ACTIVATE, 0);
                     }
                  }
               }
            }

            // Repaint item
            _InvalidateItem(m_iSelectedRow, m_iSelectedCol);
         }
      }
      else 
      {
         // Clicked outside list elements; remove selection...
         _DestroyInplaceWindow();
         _InvalidateItem(m_iSelectedRow, m_iSelectedCol);
         m_iSelectedRow = m_iSelectedCol = -1;
         // Let owner know
         NMPROPERTYITEM nmh = { m_hWnd, GetDlgCtrlID(), PIN_SELCHANGED, NULL };
         ::SendMessage(GetParent(), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh);
      }
      return lRes;
   }

   LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
   {      
      switch( wParam ) {
      case VK_F2:
      case VK_SPACE:
      case VK_RETURN:
         if( _IsValidSelection() ) {
            IProperty* prop = GetProperty(m_iSelectedRow, m_iSelectedCol);
            ATLASSERT(prop);
            if( prop->IsEnabled() ) {
               _SpawnInplaceWindow(prop, m_iSelectedRow, m_iSelectedCol);
               prop->Activate(PACT_SPACE, 0);
               _InvalidateItem(m_iSelectedRow, m_iSelectedCol);
            }
         }
         return 0;
      case VK_TAB:
         if( (m_di.dwExtStyle & PGS_EX_TABNAVIGATION) != 0 ) {
            SendMessage(WM_USER_PROP_NAVIGATE, VK_TAB);
         }
         else {
            ::PostMessage(GetParent(), WM_NEXTDLGCTL, ::GetKeyState(VK_SHIFT) < 0 ? 1 : 0, (LPARAM) FALSE);
         }
         return 0;
      case VK_LEFT:
      case VK_RIGHT:
         SendMessage(WM_USER_PROP_NAVIGATE, wParam);
         return 0;
      case VK_DELETE:
         if( _IsValidSelection() ) {
            IProperty* prop = GetProperty(m_iSelectedRow, m_iSelectedCol);
            ATLASSERT(prop);
            // We support pressing DELETE key on an edit control as well
            // as typing stuff (see OnChar() handler)
            if( prop->GetKind() == PROPKIND_EDIT && prop->IsEnabled() ) {
               if( _SpawnInplaceWindow(prop, m_iSelectedRow, m_iSelectedCol) ) {
                  prop->Activate(PACT_SPACE, 0);
                  // Simulate typing in the inplace editor...
                  ::SendMessage(m_hwndInplace, WM_KEYDOWN, wParam, 0L);
                  _InvalidateItem(m_iSelectedRow, m_iSelectedCol);
               }
            }
         }
         return 0;
      }
      bHandled = FALSE;
      return 0;
   }

   LRESULT OnChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
   {      
      // If the user is typing stuff, we should spawn an editor right away
      // and simulate the keypress in the editor-window...
      if( wParam > _T(' ') && _IsValidSelection() ) {
         IProperty* prop = GetProperty(m_iSelectedRow, m_iSelectedCol);
         ATLASSERT(prop);
         if( prop->IsEnabled() ) {
            if( _SpawnInplaceWindow(prop, m_iSelectedRow, m_iSelectedCol) ) {
               prop->Activate(PACT_SPACE, 0);
               // Simulate typing in the inplace editor...
               ::SendMessage(m_hwndInplace, WM_CHAR, wParam, 1L);
               _InvalidateItem(m_iSelectedRow, m_iSelectedCol);
            }
         }
         return 0;
      }
      if( wParam == _T(' ') ) return 0;
      bHandled = FALSE;
      return 0;
   }

   LRESULT OnGetDlgCode(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {      
      return DefWindowProc() | DLGC_WANTALLKEYS;
   }

   LRESULT OnSettingChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      // Standard colors
      m_di.clrText = ::GetSysColor(COLOR_WINDOWTEXT);
      m_di.clrBack = ::GetSysColor(COLOR_WINDOW);
      m_di.clrSelText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
      m_di.clrSelBack = ::GetSysColor(COLOR_HIGHLIGHT);
      m_di.clrDisabled = ::GetSysColor(COLOR_GRAYTEXT);
      m_di.clrDisabledBack = ::GetSysColor(COLOR_BTNFACE);
      // Border
      m_di.clrBorder = ::GetSysColor(COLOR_BTNFACE);
      if( !m_BorderPen.IsNull() ) m_BorderPen.DeleteObject();
      m_di.Border = m_BorderPen.CreatePen(PS_SOLID, 1, m_di.clrBorder);
      // Fonts
      if( !m_TextFont.IsNull() ) m_TextFont.DeleteObject();
      if( !m_CategoryFont.IsNull() ) m_CategoryFont.DeleteObject();
      LOGFONT lf = { 0 };
      HFONT hFont = (HFONT) ::SendMessage(GetParent(), WM_GETFONT, 0, 0);
      if( hFont == NULL ) hFont = AtlGetDefaultGuiFont();
      ::GetObject(hFont, sizeof(lf), &lf);
      m_di.TextFont = m_TextFont.CreateFontIndirect(&lf);
      SetFont(m_di.TextFont);
      lf.lfWeight += FW_BOLD;
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

   LRESULT OnSelChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
   {
      LPNMLISTVIEW pnmlv = (LPNMLISTVIEW) pnmh;
      // No editor, please
      _DestroyInplaceWindow();
      // Only items with selection changes...
      if( (pnmlv->uNewState & LVIS_SELECTED) == 0 ) return 0;
      // Process changed item
      int iItem = pnmlv->iItem;
      if( iItem != -1 ) {
         m_iSelectedRow = iItem;
         if( m_iSelectedCol == -1 ) m_iSelectedCol = 0;
         if( _IsAppendActionItem(iItem) ) m_iSelectedCol = 0;
         _InvalidateItem(m_iSelectedRow, m_iSelectedCol);
         // Let owner know
         IProperty* prop = GetProperty(m_iSelectedRow, m_iSelectedCol);
         ATLASSERT(prop);
         NMPROPERTYITEM nmh = { m_hWnd, GetDlgCtrlID(), PIN_SELCHANGED, prop };
         ::SendMessage(GetParent(), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh);
      }
      bHandled = FALSE;
      return 0;
   }

   LRESULT OnDeleteItem(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
   {
      ATLASSERT(m_nColumns>0);
      LPNMLISTVIEW pnmlv = (LPNMLISTVIEW) pnmh;      
      // Destroy in-place editor if active
      _DestroyInplaceWindow();
      if( pnmlv->iItem == m_iSelectedRow && m_iSelectedRow != -1 ) SelectItem(-1, -1);
      // Free sub-item place-holder array
      ATLASSERT(pnmlv->lParam);
      IProperty** props = reinterpret_cast<IProperty**>(pnmlv->lParam);
      for( int i = 0; i < m_nColumns; i++ ) if( props[i] ) delete props[i];
      delete [] props;
      return 0;
   }

   LRESULT OnNavigate(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      if( (m_di.dwExtStyle & PGS_EX_NOSHEETNAVIGATION) != 0 ) return 0;
      if( !_IsValidSelection() ) return 0;
      // Navigate in the grid control
      switch( wParam ) {
      case VK_TAB:
         SendMessage(WM_USER_PROP_NAVIGATE, VK_RIGHT);
         break;
      case VK_LEFT:
         if( _IsAppendActionItem(m_iSelectedRow ) ) break;
         if( m_iSelectedRow >= 0 && m_iSelectedCol > 0 ) {
            // Can we navigate?
            // Navigate
            m_iSelectedCol--;
            // Let owner know
            IProperty* prop = GetProperty(m_iSelectedRow, m_iSelectedCol);
            ATLASSERT(prop);
            NMPROPERTYITEM nmh = { m_hWnd, GetDlgCtrlID(), PIN_SELCHANGED, prop };
            ::SendMessage(GetParent(), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh);
            // Repaint new and old item
            _InvalidateItem(m_iSelectedRow, m_iSelectedCol + 1);
            _InvalidateItem(m_iSelectedRow, m_iSelectedCol);
         }
         break;
      case VK_RIGHT:
         if( _IsAppendActionItem(m_iSelectedRow ) ) break;
         if( m_iSelectedRow >= 0 && m_iSelectedCol < m_nColumns - 1 ) {
            // Can we navigate?
            // Navigate
            m_iSelectedCol++;
            // Let owner know
            IProperty* prop = GetProperty(m_iSelectedRow, m_iSelectedCol);
            ATLASSERT(prop);
            NMPROPERTYITEM nmh = { m_hWnd, GetDlgCtrlID(), PIN_SELCHANGED, prop };
            ::SendMessage(GetParent(), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh);
            // Repaint new and old item
            _InvalidateItem(m_iSelectedRow, m_iSelectedCol - 1);
            _InvalidateItem(m_iSelectedRow, m_iSelectedCol);
         }
         break;
      case VK_UP:
         if( m_iSelectedRow > 0 ) SelectItem(m_iSelectedRow - 1, m_iSelectedCol);
         break;
      case VK_DOWN:
         if( m_iSelectedRow < GetItemCount() - 1 ) SelectItem(m_iSelectedRow + 1, m_iSelectedCol);
         break;
      }
      return 0;
   }

   LRESULT OnUpdateProperty(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
   {
      // Updates a property value using an active editor window.
      // The editor window uses this message to update the attached property class.
      // Parameters:
      //   LPARAM = Window (HWND)
      HWND hWnd = reinterpret_cast<HWND>(lParam);     
      ATLASSERT(::IsWindow(hWnd));
      if( !::IsWindow(hWnd) || (m_iInplaceRow == -1) || (m_iInplaceCol == -1) ) return 0;
      ATLASSERT(hWnd==m_hwndInplace);
      IProperty* prop = GetProperty(m_iInplaceRow, m_iInplaceCol);
      if( prop == NULL ) return 0;
      // Ask owner about change
      NMPROPERTYITEM nmh = { m_hWnd, GetDlgCtrlID(), PIN_ITEMCHANGING, prop };
      if( ::SendMessage(GetParent(), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh) == 0 ) {
         // Commit change
         if( !prop->SetValue(hWnd) ) {
            ::MessageBeep((UINT)-1);
            return 0;
         }
         // Let owner know
         nmh.hdr.code = PIN_ITEMCHANGED;
         ::SendMessage(GetParent(), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh);
         // Repaint item
         _InvalidateItem(m_iInplaceRow, m_iInplaceCol);
      }
      // Destroy in-place control...
      _DestroyInplaceWindow();
      return 0;
   }

   LRESULT OnCancelProperty(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
   {
      // Restores a property value using an active editor window.
      // The editor window uses this message to restore the attached property class
      // with it original value.
      // Parameters:
      //   LPARAM = Window (HWND)
      HWND hWnd = reinterpret_cast<HWND>(lParam);     
      ATLASSERT(::IsWindow(hWnd));
      if( !::IsWindow(hWnd) || (m_iInplaceRow == -1) || (m_iInplaceCol == -1) ) return 0;
      ATLASSERT(hWnd==m_hwndInplace);
      // NOTE: The list-item already has the original value; it has only been
      //       obscured by the editor window which it was shown. All we need
      //       to do, is to repaint the list control.
      // Repaint item
      _InvalidateItem(m_iInplaceRow, m_iInplaceCol);
      // Destroy in-place control...
      _DestroyInplaceWindow();
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
         // Set new value.
         // NOTE: Do not call this from IProperty::SetValue(VARIANT*) = endless loop
         if( !prop->SetValue(*pVariant) ) {
            ::MessageBeep((UINT)-1);
            return 0;
         }
         // Let owner know
         nmh.hdr.code = PIN_ITEMCHANGED;
         ::SendMessage(GetParent(), WM_NOTIFY, nmh.hdr.idFrom, (LPARAM) &nmh);
      }
      // Locate the updated list item
      int iItem = -1, iSubItem = -1;
      if( !FindProperty(prop, iItem, iSubItem) ) return 0;
      // Repaint item
      _InvalidateItem(iItem, iSubItem);
      // Recycle in-place control so it displays the new value
      if( (iItem != -1) && (iItem == m_iInplaceRow) ) _SpawnInplaceWindow(prop, m_iInplaceRow, m_iInplaceCol);
      return 0;
   }

   // Header messages

   LRESULT OnHeaderChanging(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled)
   {
      // HACK: With the PGS_EX_ADDITEMATEND style we need to
      //       repaint the last item to avoid a paint bug.
      //       Code supplied by Ludvig A. Norin.
      if( (m_di.dwExtStyle & PGS_EX_ADDITEMATEND) != 0 ) {
         RECT rcItem = { 0 };
         GetItemRect(GetItemCount(), &rcItem, LVIR_BOUNDS);
         InvalidateRect(&rcItem, FALSE);
      }
      // Destroy in-place window before dragging header control
      _DestroyInplaceWindow();
      bHandled = FALSE;
      return 0;
   }

   LRESULT OnHeaderDblClick(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
   {
      // TODO
      return 0;
   }

   // Custom painting

   DWORD OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
   {
      return CDRF_NOTIFYITEMDRAW;  // We need per-item notifications
   }

   DWORD OnItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW /*lpNMCustomDraw*/)
   {
      return CDRF_NOTIFYSUBITEMDRAW;  // We need per-subitem notifications
   }

   DWORD OnSubItemPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW lpNMCustomDraw)
   {
      T* pT = static_cast<T*>(this);
      LPNMLVCUSTOMDRAW lpNMLVCustomDraw = (LPNMLVCUSTOMDRAW) lpNMCustomDraw;
      pT->DrawItem(lpNMLVCustomDraw->nmcd.hdc, 
         (int) lpNMLVCustomDraw->nmcd.dwItemSpec, 
         lpNMLVCustomDraw->iSubItem,
         lpNMLVCustomDraw->nmcd.uItemState);
      return CDRF_SKIPDEFAULT;
   }

   // Overridables

   void DrawItem(CDCHandle dc, int iItem, int iSubItem, UINT iState)
   {
      bool bIsAppendActionItem = _IsAppendActionItem(iItem);
      if( bIsAppendActionItem && iSubItem != 0 ) return;

      IProperty* prop = GetProperty(iItem, iSubItem);
      if( prop == NULL ) return;

      // Figure out rectangle
      RECT rc = { 0 };
      _GetSubItemRect(iItem, iSubItem, &rc);

      // Draw properties
      PROPERTYDRAWINFO di = m_di;
      di.hDC = dc;
      di.rcItem = rc;

      di.state = 0;   
      if( iItem == m_iSelectedRow && iSubItem == m_iSelectedCol ) di.state |= ODS_SELECTED;
      if( iItem == m_iInplaceRow && iSubItem == m_iInplaceCol ) di.state |= ODS_COMBOBOXEDIT;
      if( !prop->IsEnabled() ) di.state |= ODS_DISABLED;

      if( iItem == m_iSelectedRow ) {
         // Full row-select feature
         if( (m_di.dwExtStyle & PGS_EX_FULLROWSELECT) != 0 ) {
            di.clrBack = di.clrDisabledBack;
         }
         // Is this the selected sub-item
         if( iSubItem == m_iSelectedCol ) {
            di.state |= ODS_SELECTED;
            // Selection inverted feature
            if( (m_di.dwExtStyle & PGS_EX_INVERTSELECTION) != 0 ) {
               di.clrBack = di.clrSelBack;
               di.clrText = di.clrSelText;
            }
         }
      }

      // A disabled ListView control is painted grey on Win2000
      // but the text remains black...
      DWORD dwStyle = GetStyle();
      if( (dwStyle & WS_DISABLED) != 0 ) di.clrBack = di.clrDisabledBack;

      // Erase background
      dc.FillSolidRect(&rc, di.clrBack);

      // Draw item
      HFONT hOldFont = dc.SelectFont(di.TextFont);
      prop->DrawValue(di);
      dc.SelectFont(hOldFont);

      // Draw borders
      if( (di.dwExtStyle & PGS_EX_NOGRID) == 0 ) {
         CPen pen;
         pen.CreatePen(PS_SOLID, 1, di.clrBorder);
         HPEN hOldPen = dc.SelectPen(pen);
         dc.MoveTo(rc.left, rc.bottom-1);
         dc.LineTo(rc.right - 1, rc.bottom - 1);
         dc.LineTo(rc.right - 1, rc.top - 1);
         dc.SelectPen(hOldPen);
      }

      // Paint focus rectangle
      if( (::GetFocus() == m_hWnd && (iState & CDIS_FOCUS) != 0) || (dwStyle & LVS_SHOWSELALWAYS) != 0 ) {
         if( (iItem == m_iSelectedRow) && (iSubItem == m_iSelectedCol || bIsAppendActionItem ) ) {
            if( iItem != m_iInplaceRow && iSubItem != m_iInplaceCol ) {
               if( (GetExtendedListViewStyle() & LVS_EX_GRIDLINES) != 0 ) {
                  rc.left++; rc.right--; rc.bottom--;
               }
               CBrush brush;
               brush.CreateSolidBrush(di.clrSelBack);
               dc.FrameRect(&rc, brush);
            }
         }
      }
   }
};


class CPropertyGridCtrl : public CPropertyGridImpl<CPropertyGridCtrl>
{
public:
   DECLARE_WND_SUPERCLASS(_T("WTL_PropertyGrid"), GetWndClassName())
};


#endif // __PROPERTYGRID__H
