#pragma once

#include "afxdialogex.h"
#include <vector>
#include <string>
// #include "resource.h" // Assuming resource IDs are defined elsewhere or via stdafx.h/pch.h

// Placeholder Resource IDs - to be defined by the worker/resource file
#ifndef IDD_CHECK_RESULTS_DIALOG
#define IDD_CHECK_RESULTS_DIALOG 1010 // Placeholder
#endif
#ifndef IDC_LIST_INVALID_SHORTCUTS
#define IDC_LIST_INVALID_SHORTCUTS 1011 // Placeholder
#endif
#ifndef IDC_LIST_EMPTY_FOLDERS
#define IDC_LIST_EMPTY_FOLDERS 1012 // Placeholder
#endif
#ifndef ID_BUTTON_DELETE_SELECTED_CHECK // Often, standard IDOK is used or a custom one
#define ID_BUTTON_DELETE_SELECTED_CHECK IDOK // Placeholder, can be e.g., 1013
#endif
#ifndef IDC_BUTTON_SELECT_ALL_SHORTCUTS
#define IDC_BUTTON_SELECT_ALL_SHORTCUTS 1014 // Placeholder
#endif
#ifndef IDC_BUTTON_DESELECT_ALL_SHORTCUTS
#define IDC_BUTTON_DESELECT_ALL_SHORTCUTS 1015 // Placeholder
#endif
#ifndef IDC_BUTTON_SELECT_ALL_FOLDERS
#define IDC_BUTTON_SELECT_ALL_FOLDERS 1016 // Placeholder
#endif
#ifndef IDC_BUTTON_DESELECT_ALL_FOLDERS
#define IDC_BUTTON_DESELECT_ALL_FOLDERS 1017 // Placeholder
#endif


class CCheckResultsDlg : public CDialogEx {
    DECLARE_DYNAMIC(CCheckResultsDlg)

public:
    CCheckResultsDlg(const std::vector<std::wstring>& invalidShortcuts,
                     const std::vector<std::wstring>& emptyFolders,
                     CWnd* pParent = nullptr);
    virtual ~CCheckResultsDlg();

    enum { IDD = IDD_CHECK_RESULTS_DIALOG };

    std::vector<std::wstring> GetSelectedItemsToDelete() const;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

private:
    CListCtrl m_listCtrlInvalidShortcuts;
    CListCtrl m_listCtrlEmptyFolders;

    // Store references to the data passed in constructor to avoid copying large vectors
    const std::vector<std::wstring>& m_initialInvalidShortcuts;
    const std::vector<std::wstring>& m_initialEmptyFolders;

    // This vector will be populated when the user clicks "Delete Selected"
    std::vector<std::wstring> m_selectedItemsToDelete;

    void PopulateListCtrl(CListCtrl& listCtrl, const std::vector<std::wstring>& items);
    void SelectAllItems(CListCtrl& listCtrl, bool select);

public:
    // Using ID_BUTTON_DELETE_SELECTED_CHECK as the ID for the "Delete Selected" button
    afx_msg void OnBnClickedDeleteSelected();
    afx_msg void OnBnClickedSelectAllShortcuts();
    afx_msg void OnBnClickedDeselectAllShortcuts();
    afx_msg void OnBnClickedSelectAllFolders();
    afx_msg void OnBnClickedDeselectAllFolders();
    // Default OnCancel is fine (maps to IDCANCEL or ESC key)
};
