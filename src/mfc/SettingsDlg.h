#pragma once

#include "afxdialogex.h"
#include "../../ConfigManager/ConfigManager.h" // For ConfigManager
#include "../../WebClassifier/WebClassifier.h"   // For WebClassifier (clearCache)
// Assuming resource.h will be available and correctly referenced by the build environment
// For example, it might be included via stdafx.h or pch.h, or directly if needed.
// #include "resource.h" // Placeholder, actual ID definitions are external to this task

// Forward declare resource IDs to be defined by the worker/resource file
// This is a common practice if resource.h isn't directly included or available during this step.
// The actual values will come from the project's resource file.
#ifndef IDD_SETTINGS_DIALOG
#define IDD_SETTINGS_DIALOG 1000 // Placeholder ID
#endif
#ifndef IDC_CHECK_ENABLE_WEB_CLASSIFIER
#define IDC_CHECK_ENABLE_WEB_CLASSIFIER 1001 // Placeholder ID
#endif
#ifndef IDC_CHECK_USE_RECYCLE_BIN
#define IDC_CHECK_USE_RECYCLE_BIN 1002 // Placeholder ID
#endif
#ifndef IDC_LIST_EXCLUDED_PATHS
#define IDC_LIST_EXCLUDED_PATHS 1003 // Placeholder ID
#endif
#ifndef IDC_EDIT_EXCLUDED_PATH_INPUT
#define IDC_EDIT_EXCLUDED_PATH_INPUT 1004 // Placeholder ID
#endif
#ifndef IDC_BUTTON_ADD_EXCLUDED_PATH
#define IDC_BUTTON_ADD_EXCLUDED_PATH 1005 // Placeholder ID
#endif
#ifndef IDC_BUTTON_REMOVE_EXCLUDED_PATH
#define IDC_BUTTON_REMOVE_EXCLUDED_PATH 1006 // Placeholder ID
#endif
#ifndef IDC_BUTTON_BROWSE_EXCLUDED_PATH
#define IDC_BUTTON_BROWSE_EXCLUDED_PATH 1007 // Placeholder ID
#endif
#ifndef IDC_BUTTON_CLEAR_CACHE
#define IDC_BUTTON_CLEAR_CACHE 1008 // Placeholder ID
#endif


class CSettingsDlg : public CDialogEx {
    DECLARE_DYNAMIC(CSettingsDlg)

public:
    CSettingsDlg(CWnd* pParent = nullptr);   // standard constructor
    virtual ~CSettingsDlg();

// Dialog Data - Resource ID will be defined by the worker, e.g., IDD_SETTINGS_DIALOG
    enum { IDD = IDD_SETTINGS_DIALOG }; // Uses placeholder IDD

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

private:
    // Controls
    CButton m_checkEnableWebClassifier;
    CButton m_checkUseRecycleBin;
    CListBox m_listExcludedPaths; // Using CListBox for simplicity
    CEdit m_editExcludePathInput;
    CButton m_btnAddPath;
    CButton m_btnRemovePath;
    CButton m_btnBrowsePath;
    CButton m_btnClearCache;

    // Helper to load paths into the listbox
    void PopulateExcludedPathsList();
    // Helper to get paths from the listbox
    std::vector<std::wstring> GetPathsFromExcludedList();


public:
    // Action Handlers
    afx_msg void OnOK();
    afx_msg void OnBnClickedClearCache();
    afx_msg void OnBnClickedAddPath();
    afx_msg void OnBnClickedRemovePath();
    afx_msg void OnBnClickedBrowsePath();
};
