#include "stdafx.h" // Or pch.h, depending on project settings
#include "SettingsDlg.h"
// #include "resource.h" // Assuming IDs are defined elsewhere or via stdafx.h/pch.h
                       // The placeholder IDs are in SettingsDlg.h for now.
#include "afxdialogex.h"
#include <shlobj.h>    // For SHBrowseForFolder

// Ensure ConfigManager and WebClassifier headers are accessible
// These paths are relative to this .cpp file's location (src/mfc/)
#include "../../ConfigManager/ConfigManager.h"
#include "../../WebClassifier/WebClassifier.h"


IMPLEMENT_DYNAMIC(CSettingsDlg, CDialogEx)

CSettingsDlg::CSettingsDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(CSettingsDlg::IDD, pParent) { // IDD is from SettingsDlg.h (placeholder)
}

CSettingsDlg::~CSettingsDlg() {}

void CSettingsDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    // Associate controls with member variables - IDs are placeholders from SettingsDlg.h
    DDX_Control(pDX, IDC_CHECK_ENABLE_WEB_CLASSIFIER, m_checkEnableWebClassifier);
    DDX_Control(pDX, IDC_CHECK_USE_RECYCLE_BIN, m_checkUseRecycleBin);
    DDX_Control(pDX, IDC_LIST_EXCLUDED_PATHS, m_listExcludedPaths);
    DDX_Control(pDX, IDC_EDIT_EXCLUDED_PATH_INPUT, m_editExcludePathInput);
    DDX_Control(pDX, IDC_BUTTON_ADD_EXCLUDED_PATH, m_btnAddPath);
    DDX_Control(pDX, IDC_BUTTON_REMOVE_EXCLUDED_PATH, m_btnRemovePath);
    DDX_Control(pDX, IDC_BUTTON_BROWSE_EXCLUDED_PATH, m_btnBrowsePath);
    DDX_Control(pDX, IDC_BUTTON_CLEAR_CACHE, m_btnClearCache);
}

BEGIN_MESSAGE_MAP(CSettingsDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BUTTON_CLEAR_CACHE, &CSettingsDlg::OnBnClickedClearCache)
    ON_BN_CLICKED(IDC_BUTTON_ADD_EXCLUDED_PATH, &CSettingsDlg::OnBnClickedAddPath)
    ON_BN_CLICKED(IDC_BUTTON_REMOVE_EXCLUDED_PATH, &CSettingsDlg::OnBnClickedRemovePath)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE_EXCLUDED_PATH, &CSettingsDlg::OnBnClickedBrowsePath)
    // ON_COMMAND(IDOK, &CSettingsDlg::OnOK) // Default OnOK is fine if we call base class and have an OnOK handler
END_MESSAGE_MAP()

BOOL CSettingsDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();
    ConfigManager& cfg = ConfigManager::getInstance();

    m_checkEnableWebClassifier.SetCheck(cfg.isWebClassifierEnabled() ? BST_CHECKED : BST_UNCHECKED);
    m_checkUseRecycleBin.SetCheck(cfg.shouldUseRecycleBin() ? BST_CHECKED : BST_UNCHECKED);
    PopulateExcludedPathsList();

    return TRUE;  // return TRUE unless you set the focus to a control
}

void CSettingsDlg::PopulateExcludedPathsList() {
    m_listExcludedPaths.ResetContent(); // Clear existing items
    ConfigManager& cfg = ConfigManager::getInstance();
    const auto& paths = cfg.getExcludedPaths();
    for (const auto& path : paths) {
        m_listExcludedPaths.AddString(path.c_str());
    }
}

std::vector<std::wstring> CSettingsDlg::GetPathsFromExcludedList() {
    std::vector<std::wstring> paths;
    for (int i = 0; i < m_listExcludedPaths.GetCount(); ++i) {
        CString itemText;
        m_listExcludedPaths.GetText(i, itemText);
        paths.push_back(itemText.GetString());
    }
    return paths;
}

void CSettingsDlg::OnOK() {
    ConfigManager& cfg = ConfigManager::getInstance();

    cfg.setWebClassifierEnabled(m_checkEnableWebClassifier.GetCheck() == BST_CHECKED);
    cfg.setShouldUseRecycleBin(m_checkUseRecycleBin.GetCheck() == BST_CHECKED);

    std::vector<std::wstring> paths = GetPathsFromExcludedList();
    cfg.setExcludedPaths(paths); // This will call saveSettings in ConfigManager

    // ConfigManager::getInstance().saveSettings(); // Not strictly necessary if setters in ConfigManager save already.
                                                 // Based on previous subtask, setters in ConfigManager DO save.

    CDialogEx::OnOK(); // Call base class to close dialog
}

void CSettingsDlg::OnBnClickedClearCache() {
    if (AfxMessageBox(L"Are you sure you want to clear the web classifier cache?", MB_YESNO | MB_ICONQUESTION) == IDYES) {
        WebClassifier webCls; // Create a temporary instance
        // Ensure WebClassifier includes Logging.h if clearCache uses LOG_ macros directly
        // and that WebClassifier is linked correctly if its methods are in a separate .cpp
        webCls.clearCache();
        AfxMessageBox(L"Web classifier cache cleared.", MB_OK | MB_ICONINFORMATION);
    }
}

void CSettingsDlg::OnBnClickedAddPath() {
    CString pathToAdd;
    m_editExcludePathInput.GetWindowText(pathToAdd);
    if (!pathToAdd.IsEmpty()) {
        bool found = false;
        for (int i = 0; i < m_listExcludedPaths.GetCount(); ++i) {
            CString currentItemText;
            m_listExcludedPaths.GetText(i, currentItemText);
            if (currentItemText.CompareNoCase(pathToAdd) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            m_listExcludedPaths.AddString(pathToAdd);
            m_editExcludePathInput.SetWindowText(L""); // Clear input
        } else {
            AfxMessageBox(L"This path is already in the exclusion list.", MB_OK | MB_ICONINFORMATION);
        }
    } else {
        AfxMessageBox(L"Please enter a path to add.", MB_OK | MB_ICONWARNING);
    }
}

void CSettingsDlg::OnBnClickedRemovePath() {
    int selectedIndex = m_listExcludedPaths.GetCurSel();
    if (selectedIndex != LB_ERR) {
        m_listExcludedPaths.DeleteString(selectedIndex);
    } else {
        AfxMessageBox(L"Please select a path to remove.", MB_OK | MB_ICONWARNING);
    }
}

// Basic SHBrowseForFolder implementation
static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) {
    if (uMsg == BFFM_INITIALIZED) {
        // Send BFFM_SETSELECTION to LPCWSTR dir. lpData needs to be a LPCWSTR.
        // Example: SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)L"C:\\Program Files");
        if (lpData) { // Check if lpData is provided
             SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
        }
    }
    return 0;
}

void CSettingsDlg::OnBnClickedBrowsePath() {
    wchar_t currentPathInput[MAX_PATH];
    m_editExcludePathInput.GetWindowText(currentPathInput, MAX_PATH);

    wchar_t path[MAX_PATH];
    BROWSEINFOW bi = { 0 };
    bi.hwndOwner  = this->GetSafeHwnd(); // Set owner to the dialog
    bi.lpszTitle  = L"Select a folder to exclude...";
    bi.ulFlags    = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_USENEWUI;
    bi.lpfn       = BrowseCallbackProc;
    // Pass current input text as initial path if not empty, otherwise NULL
    bi.lParam     = (wcslen(currentPathInput) > 0) ? (LPARAM)currentPathInput : NULL;


    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);

    if (pidl != 0) {
        // Get the name of the folder and store it in path
        SHGetPathFromIDListW(pidl, path);

        IMalloc * imalloc = 0;
        if (SUCCEEDED(SHGetMalloc(&imalloc))) {
            imalloc->Free(pidl);
            imalloc->Release();
        }
        m_editExcludePathInput.SetWindowText(path);
    }
}
