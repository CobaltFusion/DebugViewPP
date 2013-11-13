// maindlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINDLG_H__4DAAD2C6_BE53_4E7A_BF77_C393A39420D3__INCLUDED_)
#define AFX_MAINDLG_H__4DAAD2C6_BE53_4E7A_BF77_C393A39420D3__INCLUDED_

#pragma once

#include "PropertyList.h"
#include "PropertyTree.h"
#include "PropertyGrid.h"
#include "ColorCombo.h"


class CMainDlg : public CDialogImpl<CMainDlg>
{
public:
   enum { IDD = IDD_MAINDLG };

   CPropertyListCtrl m_list;
   CPropertyTreeCtrl m_tree;
   CPropertyGridCtrl m_grid;
   CImageList m_images;
   CColorPickerListCtrl m_color;

   BEGIN_MSG_MAP(CMainDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
      COMMAND_ID_HANDLER(IDOK, OnOK)
      COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
      NOTIFY_HANDLER(IDC_LIST1, PIN_BROWSE, OnBrowse)
      NOTIFY_CODE_HANDLER(PIN_SELCHANGED, OnSelChanged);
      NOTIFY_CODE_HANDLER(PIN_ITEMCHANGED, OnItemChanged);
      NOTIFY_CODE_HANDLER(PIN_ADDITEM, OnAddItem);
      REFLECT_NOTIFICATIONS()
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      // center the dialog on the screen
      CenterWindow();

      // set icons
      HICON hIcon = (HICON) ::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
         IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
      SetIcon(hIcon, TRUE);
      HICON hIconSmall = (HICON) ::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
         IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
      SetIcon(hIconSmall, FALSE);

      _InitGrid();
      _InitList();
      _InitTree();

      return TRUE;
   }

   LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {
      m_color.DestroyWindow();
      bHandled = FALSE;
      return 0;
   }

   void _InitGrid()
   {
      m_grid.SubclassWindow(GetDlgItem(IDC_LISTVIEW1));

      m_grid.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 200, 0);
      m_grid.InsertColumn(1, _T("Skinny"), LVCFMT_LEFT, 80, 0);
      m_grid.InsertColumn(2, _T("Brainy"), LVCFMT_LEFT, 70, 0);
      m_grid.InsertColumn(3, _T("Animal"), LVCFMT_LEFT, 90, 0);

      //m_grid.SetExtendedGridStyle(PGS_EX_SINGLECLICKEDIT);
      m_grid.SetExtendedGridStyle(PGS_EX_SINGLECLICKEDIT | PGS_EX_ADDITEMATEND);

      m_grid.InsertItem(0, PropCreateReadOnlyItem(_T(""), _T("Raymond")));
      m_grid.SetSubItem(0, 1, PropCreateSimple(_T(""), false));
      m_grid.SetSubItem(0, 2, PropCreateCheckButton(_T(""), true));
      m_grid.SetSubItem(0, 3, PropCreateSimple(_T(""), _T("Monkey")));

      HPROPERTY hBeth = PropCreateReadOnlyItem(_T(""), _T("Beth"));
      m_grid.InsertItem(1, hBeth);
      m_grid.SetSubItem(1, 1, PropCreateSimple(_T(""), true));
      m_grid.SetSubItem(1, 2, PropCreateCheckButton(_T(""), false));
      m_grid.SetSubItem(1, 3, PropCreateSimple(_T(""), _T("Frog")));

      m_grid.InsertItem(2, PropCreateReadOnlyItem(_T(""), _T("Caroline")));
      m_grid.SetSubItem(2, 1, PropCreateSimple(_T(""), false));
      m_grid.SetSubItem(2, 2, PropCreateCheckButton(_T(""), false));
      HPROPERTY hHorse = PropCreateSimple(_T(""), _T("Horse"));
      m_grid.SetSubItem(2, 3, hHorse);

      ATLASSERT(hHorse == m_grid.GetProperty(2,3));
      m_grid.SetItemEnabled(hHorse, FALSE);

      // Sample on how to deal with property handles vs. property classes
      // This is a nasty, unfriendly C++, design, and I'm truely ashamed of it.
      //   It only works because we KNOW that it really is a
      //   CPropertyReadOnlyItem class which gets constructed...
      CPropertyReadOnlyItem* pProp = reinterpret_cast<CPropertyReadOnlyItem*>(hBeth);
      pProp->SetTextColor(RGB(0,0,255));
      HICON hIcon = (HICON) ::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
         IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
      pProp->SetIcon( hIcon );
   }

   void _InitList()
   {
      m_list.SubclassWindow(GetDlgItem(IDC_LIST1));

      SYSTEMTIME stEmpty = { 0 };

      m_list.SetExtendedListStyle(PLS_EX_CATEGORIZED | PLS_EX_SHOWSELALWAYS | PLS_EX_SINGLECLICKEDIT);
      m_list.AddItem( PropCreateCategory(_T("Appearance")) );
      m_list.AddItem( PropCreateSimple(_T("Name"), _T("Form1")) );
      m_list.AddItem( PropCreateSimple(_T("X"), 123L) );
      m_list.AddItem( PropCreateSimple(_T("Y"), 456L) );
      m_list.AddItem( PropCreateCategory(_T("Behaviour")) );
      m_list.AddItem( PropCreateSimple(_T("Enabled"), false) );
      m_list.AddItem( PropCreateFileName(_T("Picture"), _T("C:\\Temp\\Test.bmp")) );

      m_color.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_BORDER);
      m_color.SetParent(NULL);
      m_color.AddColor(0, RGB(255,255,255));
      m_color.AddColor(1, RGB(255,0,0));
      m_color.AddColor(2, RGB(0,255,0));
      m_color.AddColor(3, RGB(0,0,255));
      m_list.AddItem( PropCreateComboControl(_T("Color"), m_color, (int) RGB(0,255,0)) );

      m_list.AddItem( PropCreateDate(_T("Date"), stEmpty) );
      HPROPERTY hDisabled = m_list.AddItem( PropCreateSimple(_T("Disabled"), _T("Form1")) );

      ATLASSERT(hDisabled==m_list.FindProperty(_T("Disabled")));
      m_list.SetItemEnabled(hDisabled, FALSE);
   }

   void _InitTree()
   {
      m_images.Create(IDB_PROPERTYTREE, 16, 1, RGB(255,0,255));

      m_tree.SubclassWindow( GetDlgItem(IDC_TREE1) );

      m_tree.SetImageList(m_images, TVSIL_NORMAL);
      m_tree.SetExtendedTreeStyle(PTS_EX_NOCOLLAPSE | PTS_EX_SINGLECLICKEDIT);

      LPCTSTR pList[] = { _T("White"), _T("Brown"), _T("Pink"), _T("Yellow"), NULL };
      HTREEITEM hItem;
      HTREEITEM hDisabled;
      hItem = m_tree.InsertItem( PropCreateReadOnlyItem(_T("Accessibility")), 13, 13, TVI_ROOT);
         m_tree.InsertItem( PropCreateCheckmark(_T("Always expand ALT text for images")), 0, 0, hItem);
         m_tree.InsertItem( PropCreateCheckmark(_T("Move system caret with focus changes")), 0, 0, hItem);
      hItem = m_tree.InsertItem( PropCreateReadOnlyItem(_T("When searching")), 10, 10, TVI_ROOT);
         m_tree.InsertItem( PropCreateOptionCheck(_T("Display results, and go to most likely site")), 0, 0, hItem);
         m_tree.InsertItem( PropCreateOptionCheck(_T("Do not search from the Address bar"), true), 0, 0, hItem);
      hItem = m_tree.InsertItem( PropCreateReadOnlyItem(_T("Misc")), 13, 13, TVI_ROOT);
         m_tree.InsertItem( PropCreateSimple(_T("Name"), _T("Donald")), 11, 11, hItem);
         m_tree.InsertItem( PropCreateSimple(_T("Male"), true), 12, 12, hItem);
         m_tree.InsertItem( PropCreateList(_T("Skin"), pList), 12, 12, hItem);
         hDisabled = m_tree.InsertItem( PropCreateSimple(_T("Disabled"), true), 12, 12, hItem);

      m_tree.EnableItem(hDisabled, FALSE);
   }

   LRESULT OnBrowse(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
   {
      LPNMPROPERTYITEM nmp = (LPNMPROPERTYITEM) pnmh;
      LPCTSTR lpcstrFilter = 
         _T("Bitmap Files (*.bmp)\0*.bmp\0")
         _T("All Files (*.*)\0*.*\0")
         _T("");
      CFileDialog dlg(TRUE, _T("bmp"), _T("C:\\Temp\\Image.bmp"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, lpcstrFilter);
      int nRet = dlg.DoModal();
      if(nRet == IDOK) {
         CComVariant v(dlg.m_ofn.lpstrFile);
         m_list.SetItemValue(nmp->prop, &v);
      }
      return 0;
   }

   LRESULT OnAddItem(int idCtrl, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
   {
      ATLTRACE(_T("OnAddItem - Ctrl: %d\n"), idCtrl); idCtrl;

      int i = m_grid.InsertItem(-1, PropCreateReadOnlyItem(_T(""), _T("Dolly")));
      m_grid.SetSubItem(i, 1, PropCreateSimple(_T(""), true));
      m_grid.SetSubItem(i, 2, PropCreateCheckButton(_T(""), false));
      m_grid.SetSubItem(i, 3, PropCreateSimple(_T(""), _T("")));
      m_grid.SelectItem(i);
      return 0;
   }

   LRESULT OnSelChanged(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/)
   {
      LPNMPROPERTYITEM pnpi = (LPNMPROPERTYITEM) pnmh;
      if( pnpi->prop==NULL ) return 0;
      TCHAR szValue[100] = { 0 };      
      pnpi->prop->GetDisplayValue(szValue, sizeof(szValue)/sizeof(TCHAR));
      CComVariant vValue;
      pnpi->prop->GetValue(&vValue);
      vValue.ChangeType(VT_BSTR);
      ATLTRACE(_T("OnSelChanged - Ctrl: %d, Name: '%s', DispValue: '%s', Value: '%ls'\n"),
         idCtrl, pnpi->prop->GetName(), szValue, vValue.bstrVal); idCtrl;
      return 0;
   }

   LRESULT OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/)
   {
      LPNMPROPERTYITEM pnpi = (LPNMPROPERTYITEM) pnmh;
      TCHAR szValue[100] = { 0 };
      pnpi->prop->GetDisplayValue(szValue, sizeof(szValue)/sizeof(TCHAR));
      CComVariant vValue;
      pnpi->prop->GetValue(&vValue);
      vValue.ChangeType(VT_BSTR);
      ATLTRACE(_T("OnItemChanged - Ctrl: %d, Name: '%s', DispValue: '%s', Value: '%ls'\n"),
         idCtrl, pnpi->prop->GetName(), szValue, vValue.bstrVal); idCtrl;
      return 0;
   }

   LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CSimpleDialog<IDD_ABOUTBOX, FALSE> dlg;
      dlg.DoModal();
      return 0;
   }

   LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      // TODO: Add validation code 
      EndDialog(wID);
      return 0;
   }

   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINDLG_H__4DAAD2C6_BE53_4E7A_BF77_C393A39420D3__INCLUDED_)
