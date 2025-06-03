#include "stdafx.h" // Or pch.h
#include "CheckResultsDlg.h"
#include "afxdialogex.h"
// #include "resource.h" // Assuming IDs are defined elsewhere (e.g. via stdafx.h/pch.h or directly in SettingsDlg.h for now)
                       // The placeholder IDs are in CheckResultsDlg.h

IMPLEMENT_DYNAMIC(CCheckResultsDlg, CDialogEx)

CCheckResultsDlg::CCheckResultsDlg(const std::vector<std::wstring>& invalidShortcuts,
                                   const std::vector<std::wstring>& emptyFolders,
                                   CWnd* pParent /*=nullptr*/)
    : CDialogEx(CCheckResultsDlg::IDD, pParent), // IDD is from CheckResultsDlg.h (placeholder)
      m_initialInvalidShortcuts(invalidShortcuts),
      m_initialEmptyFolders(emptyFolders) {
    // Constructor initializes references to the passed-in data
}

CCheckResultsDlg::~CCheckResultsDlg() {}

void CCheckResultsDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    // Associate CListCtrl members with their resource IDs (placeholders from .h)
    DDX_Control(pDX, IDC_LIST_INVALID_SHORTCUTS, m_listCtrlInvalidShortcuts);
    DDX_Control(pDX, IDC_LIST_EMPTY_FOLDERS, m_listCtrlEmptyFolders);
    // Buttons are handled by message map, no DDX needed unless they are member CButton variables
    // and you need to enable/disable them or change text dynamically.
}

BEGIN_MESSAGE_MAP(CCheckResultsDlg, CDialogEx)
    // Using ID_BUTTON_DELETE_SELECTED_CHECK which is defined as IDOK in the .h for now
    ON_BN_CLICKED(ID_BUTTON_DELETE_SELECTED_CHECK, &CCheckResultsDlg::OnBnClickedDeleteSelected)
    ON_BN_CLICKED(IDC_BUTTON_SELECT_ALL_SHORTCUTS, &CCheckResultsDlg::OnBnClickedSelectAllShortcuts)
    ON_BN_CLICKED(IDC_BUTTON_DESELECT_ALL_SHORTCUTS, &CCheckResultsDlg::OnBnClickedDeselectAllShortcuts)
    ON_BN_CLICKED(IDC_BUTTON_SELECT_ALL_FOLDERS, &CCheckResultsDlg::OnBnClickedSelectAllFolders)
    ON_BN_CLICKED(IDC_BUTTON_DESELECT_ALL_FOLDERS, &CCheckResultsDlg::OnBnClickedDeselectAllFolders)
END_MESSAGE_MAP()

BOOL CCheckResultsDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();

    // Setup List Controls with Checkboxes and Full Row Select
    DWORD dwStyleShortcuts = ListView_GetExtendedListViewStyle(m_listCtrlInvalidShortcuts.m_hWnd);
    ListView_SetExtendedListViewStyle(m_listCtrlInvalidShortcuts.m_hWnd, dwStyleShortcuts | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
    m_listCtrlInvalidShortcuts.InsertColumn(0, L"Path", LVCFMT_LEFT, 400); // Adjust width as needed

    DWORD dwStyleFolders = ListView_GetExtendedListViewStyle(m_listCtrlEmptyFolders.m_hWnd);
    ListView_SetExtendedListViewStyle(m_listCtrlEmptyFolders.m_hWnd, dwStyleFolders | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
    m_listCtrlEmptyFolders.InsertColumn(0, L"Path", LVCFMT_LEFT, 400); // Adjust width as needed

    // Populate the list controls with data
    PopulateListCtrl(m_listCtrlInvalidShortcuts, m_initialInvalidShortcuts);
    PopulateListCtrl(m_listCtrlEmptyFolders, m_initialEmptyFolders);

    // By default, all items are checked (as per PopulateListCtrl and requirements)
    // If you want to ensure all are selected initially (redundant if PopulateListCtrl does it, but safe):
    // SelectAllItems(m_listCtrlInvalidShortcuts, true);
    // SelectAllItems(m_listCtrlEmptyFolders, true);

    CString title;
    title.Format(L"Desktop Check Results (%zu invalid shortcuts, %zu empty folders)",
                 m_initialInvalidShortcuts.size(), m_initialEmptyFolders.size());
    SetWindowText(title);

    // Disable OK button if both lists are empty initially.
    // The OK button is the "Delete Selected" button in this dialog's context.
    CWnd* pDeleteButton = GetDlgItem(ID_BUTTON_DELETE_SELECTED_CHECK); // or IDOK
    if (pDeleteButton) {
        if (m_initialInvalidShortcuts.empty() && m_initialEmptyFolders.empty()) {
            pDeleteButton->EnableWindow(FALSE);
        }
    }

    return TRUE;  // return TRUE unless you set the focus to a control
}

void CCheckResultsDlg::PopulateListCtrl(CListCtrl& listCtrl, const std::vector<std::wstring>& items) {
    listCtrl.DeleteAllItems(); // Clear previous items
    int index = 0;
    for (const auto& itemPath : items) {
        listCtrl.InsertItem(index, itemPath.c_str());
        listCtrl.SetCheck(index, TRUE); // Default to checked
        index++;
    }
}

void CCheckResultsDlg::SelectAllItems(CListCtrl& listCtrl, bool select) {
    for (int i = 0; i < listCtrl.GetItemCount(); ++i) {
        listCtrl.SetCheck(i, select);
    }
}

std::vector<std::wstring> CCheckResultsDlg::GetSelectedItemsToDelete() const {
    // This method returns the paths collected when "Delete Selected" was clicked.
    return m_selectedItemsToDelete;
}

void CCheckResultsDlg::OnBnClickedDeleteSelected() {
    m_selectedItemsToDelete.clear(); // Clear any previous selections

    // Collect checked items from the invalid shortcuts list
    for (int i = 0; i < m_listCtrlInvalidShortcuts.GetItemCount(); ++i) {
        if (m_listCtrlInvalidShortcuts.GetCheck(i)) {
            m_selectedItemsToDelete.push_back(m_listCtrlInvalidShortcuts.GetItemText(i, 0).GetString());
        }
    }

    // Collect checked items from the empty folders list
    for (int i = 0; i < m_listCtrlEmptyFolders.GetItemCount(); ++i) {
        if (m_listCtrlEmptyFolders.GetCheck(i)) {
            m_selectedItemsToDelete.push_back(m_listCtrlEmptyFolders.GetItemText(i, 0).GetString());
        }
    }

    // End the dialog, returning IDOK. The parent will call GetSelectedItemsToDelete().
    CDialogEx::EndDialog(IDOK);
}

void CCheckResultsDlg::OnBnClickedSelectAllShortcuts() {
    SelectAllItems(m_listCtrlInvalidShortcuts, true);
}

void CCheckResultsDlg::OnBnClickedDeselectAllShortcuts() {
    SelectAllItems(m_listCtrlInvalidShortcuts, false);
}

void CCheckResultsDlg::OnBnClickedSelectAllFolders() {
    SelectAllItems(m_listCtrlEmptyFolders, true);
}

void CCheckResultsDlg::OnBnClickedDeselectAllFolders() {
    SelectAllItems(m_listCtrlEmptyFolders, false);
}
