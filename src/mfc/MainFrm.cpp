#include "stdafx.h" // Typically included in MFC .cpp files
#include "MainFrm.h"
#include "SettingsDlg.h"
#include "CheckResultsDlg.h"
#include "../DesktopManager/DesktopChecker.h"
#include "../DesktopManager/DesktopFileOperations.h"
#include "../DesktopManager/DesktopInfo.h"
#include "../DesktopManager/DesktopSorter.h"
#include "../DesktopArrangement/DesktopLayoutManager.h" // This should bring in ScreenMetrics
#include "../ConfigManager/ConfigManager.h"
#include "../Logging/Logging.h" // Already included

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_COMMAND(IDC_BUTTON_CHECK_DESKTOP, &CMainFrame::OnCheckDesktop)
    ON_COMMAND(IDC_BUTTON_SORT_DESKTOP, &CMainFrame::OnSortDesktop)
    ON_COMMAND(IDC_BUTTON_SETTINGS, &CMainFrame::OnSettings)
END_MESSAGE_MAP()

CMainFrame::CMainFrame() {
    LOG_DEBUG(L"CMainFrame::CMainFrame() - Constructor called.");
}

CMainFrame::~CMainFrame() {
    LOG_DEBUG(L"CMainFrame::~CMainFrame() - Destructor called.");
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) {
    LOG_INFO(L"CMainFrame::OnCreate - Entered.");
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1) {
        LOG_ERROR(L"CMainFrame::OnCreate - CFrameWnd::OnCreate failed.");
        return -1;
    }

    if (!m_wndStatusBar.Create(this)) {
        LOG_ERROR(L"CMainFrame::OnCreate - Failed to create status bar.");
        return -1;
    }
    m_wndStatusBar.SetIndicators(nullptr, 0);
    m_wndStatusBar.SetPaneText(0, L"Готово");
    LOG_INFO(L"CMainFrame::OnCreate - Status bar created.");

    if (GetWindowTextLength() == 0) {
         SetWindowText(L"Умный Организатор Рабочего Стола");
         LOG_INFO(L"CMainFrame::OnCreate - Window title set manually as it was empty.");
    }

    CRect clientRect;
    GetClientRect(&clientRect);

    int btnWidth = 200;
    int btnHeight = 30;
    int spacing = 10;
    int topMargin = 10;
    int leftMargin = 10;

    if (!m_btnCheckDesktop.Create(L"Проверить рабочий стол", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(leftMargin, topMargin, leftMargin + btnWidth, topMargin + btnHeight), this, IDC_BUTTON_CHECK_DESKTOP)) {
        LOG_ERROR(L"CMainFrame::OnCreate - Failed to create Check Desktop button.");
        return -1;
    }

    if (!m_btnSortDesktop.Create(L"Сортировать рабочий стол", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(leftMargin, topMargin + (btnHeight + spacing), leftMargin + btnWidth, topMargin + (btnHeight + spacing) + btnHeight), this, IDC_BUTTON_SORT_DESKTOP)) {
        LOG_ERROR(L"CMainFrame::OnCreate - Failed to create Sort Desktop button.");
        return -1;
    }

    if (!m_btnSettings.Create(L"Настройки", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(leftMargin, topMargin + 2 * (btnHeight + spacing), leftMargin + btnWidth, topMargin + 2 * (btnHeight + spacing) + btnHeight), this, IDC_BUTTON_SETTINGS)) {
        LOG_ERROR(L"CMainFrame::OnCreate - Failed to create Settings button.");
        return -1;
    }
    LOG_INFO(L"CMainFrame::OnCreate - Buttons created successfully.");
    return 0;
}

#ifdef _DEBUG
void CMainFrame::AssertValid() const {
    CFrameWnd::AssertValid();
}
void CMainFrame::Dump(CDumpContext& dc) const {
    CFrameWnd::Dump(dc);
}
#endif //_DEBUG

void CMainFrame::OnCheckDesktop() {
    LOG_INFO(L"CMainFrame::OnCheckDesktop - Button clicked.");
    m_wndStatusBar.SetPaneText(0, L"Scanning desktop, please wait...");
    AfxGetApp()->DoWaitCursor(1); // Show wait cursor

    DesktopChecker checker;
    std::wstring desktopPath = checker.getDesktopPath(); // Assuming this method exists and is public
    if (desktopPath.empty()) {
        AfxMessageBox(L"Could not retrieve desktop path. Ensure DesktopChecker can access it.", MB_OK | MB_ICONERROR);
        m_wndStatusBar.SetPaneText(0, L"Error: Could not get desktop path.");
        AfxGetApp()->DoWaitCursor(-1); // Restore cursor
        return;
    }

    LOG_DEBUG(L"CMainFrame::OnCheckDesktop - Desktop path: " + desktopPath);
    std::vector<std::wstring> invalidShortcuts = checker.findInvalidShortcuts(desktopPath);
    LOG_DEBUG(L"CMainFrame::OnCheckDesktop - Found " + std::to_wstring(invalidShortcuts.size()) + L" invalid shortcuts.");
    std::vector<std::wstring> emptyFolders = checker.findEmptyFolders(desktopPath);
    LOG_DEBUG(L"CMainFrame::OnCheckDesktop - Found " + std::to_wstring(emptyFolders.size()) + L" empty folders.");

    AfxGetApp()->DoWaitCursor(-1); // Restore cursor

    if (invalidShortcuts.empty() && emptyFolders.empty()) {
        AfxMessageBox(L"No invalid shortcuts or empty folders found.", MB_OK | MB_ICONINFORMATION);
        m_wndStatusBar.SetPaneText(0, L"Desktop check complete. No items found.");
        return;
    }

    CCheckResultsDlg dlg(invalidShortcuts, emptyFolders, this);
    if (dlg.DoModal() == IDOK) { // IDOK is returned when "Delete Selected" is clicked in CCheckResultsDlg
        std::vector<std::wstring> itemsToDelete = dlg.GetSelectedItemsToDelete();
        if (!itemsToDelete.empty()) {
            ConfigManager& cfg = ConfigManager::getInstance();
            bool useRecycleBin = cfg.shouldUseRecycleBin();

            CString msg;
            msg.Format(L"Are you sure you want to delete %zu selected item(s)?\n%s",
                       itemsToDelete.size(),
                       useRecycleBin ? L"(Items will be moved to Recycle Bin)" : L"(Items will be PERMANENTLY DELETED)");

            if (AfxMessageBox(msg, MB_YESNO | MB_ICONWARNING) == IDYES) {
                m_wndStatusBar.SetPaneText(0, L"Deleting items...");
                AfxGetApp()->DoWaitCursor(1); // Show wait cursor

                DesktopFileOperations dfo;
                // Assuming deleteItems logs errors internally for items it fails to delete
                bool overallSuccess = dfo.deleteItems(itemsToDelete, useRecycleBin);

                AfxGetApp()->DoWaitCursor(-1); // Restore cursor

                if (overallSuccess) {
                    // Check if all items were deleted or partial success
                    // This might require deleteItems to return more detailed status,
                    // for now, we assume it returns true if it attempted all and logged errors.
                    // A more robust approach would be for deleteItems to return a count of failures or a list of failed items.
                    int successfullyDeletedCount = 0; // This is a simplification.
                                                      // We'd need more info from deleteItems for an accurate count.
                    // For now, let's assume if overallSuccess is true, all requested were deleted or attempted with logs.
                    // If deleteItems could return the number of items it failed to delete:
                    // int failedCount = dfo.getFailedDeletionsCount();
                    // if (failedCount == 0) { ... } else { ... }

                    // For this implementation, we'll rely on a general message.
                    // A more detailed feedback mechanism would be an improvement.
                    size_t initialCount = itemsToDelete.size();
                    // To give more accurate feedback, we'd need to know how many succeeded.
                    // Let's assume for now that 'overallSuccess' means all were processed, possibly with some failures logged by DFO.
                    // If `deleteItems` could return the count of successfully deleted items:
                    // int actuallyDeletedCount = dfo.deleteItems(...);
                    // msg.Format(L"%d item(s) processed for deletion.", actuallyDeletedCount);
                    // AfxMessageBox(msg, MB_OK | MB_ICONINFORMATION);

                    AfxMessageBox(L"Selected items processed for deletion. Check logs for details if any items were skipped.", MB_OK | MB_ICONINFORMATION);
                    m_wndStatusBar.SetPaneText(0, L"Deletion process complete.");

                } else { // This 'else' implies a more catastrophic failure of the deleteItems operation itself.
                    AfxMessageBox(L"The deletion process encountered a significant error. Please check the log file for details.", MB_OK | MB_ICONERROR);
                    m_wndStatusBar.SetPaneText(0, L"Deletion process failed.");
                }
            } else {
                m_wndStatusBar.SetPaneText(0, L"Deletion cancelled by user.");
                LOG_INFO(L"CMainFrame::OnCheckDesktop - Deletion cancelled by user.");
            }
        } else {
             m_wndStatusBar.SetPaneText(0, L"No items selected for deletion.");
             LOG_INFO(L"CMainFrame::OnCheckDesktop - No items were selected for deletion in the dialog.");
        }
    } else { // User clicked Cancel or closed the dialog
        m_wndStatusBar.SetPaneText(0, L"Desktop check cancelled by user.");
        LOG_INFO(L"CMainFrame::OnCheckDesktop - CheckResultsDlg cancelled by user.");
    }
}

void CMainFrame::OnSortDesktop() {
    LOG_INFO(L"CMainFrame::OnSortDesktop - Button clicked.");
    m_wndStatusBar.SetPaneText(0, L"Sorting desktop, please wait...");
    AfxGetApp()->DoWaitCursor(1); // Show wait cursor
    // Ensure COM is initialized for this thread if DesktopInfo/Sorter rely on it.
    // CSmartDesktopOrganizerApp::InitInstance likely calls AfxOleInit() or CoInitialize for the main UI thread.

    try {
        DesktopInfo dInfo; // Constructor calls CoInitializeEx
        DesktopChecker checker; // Create a DesktopChecker instance
        // DesktopSorter's constructor creates WebClassifier, which also calls CoInitializeEx.
        // This nested CoInitializeEx is fine if they are apartment threaded and on the same thread.
        DesktopSorter dSorter;
        DesktopLayoutManager layoutManager;

        LOG_INFO(L"CMainFrame::OnSortDesktop - Getting screen metrics and desktop items...");
        m_wndStatusBar.SetPaneText(0, L"Gathering desktop information...");
        AfxGetApp()->PumpMessages(); // Allow UI to update status

        int screenW = dInfo.getScreenWidth();
        int screenH = dInfo.getScreenHeight();
        float dpiScale = dInfo.getPrimaryMonitorDpiScale();

        ScreenMetrics screenMetrics(screenW, screenH, dpiScale);


        std::wstring desktopPath = checker.getDesktopPath(); // Use DesktopChecker instance
        if (desktopPath.empty()) {
            LOG_ERROR(L"CMainFrame::OnSortDesktop - Failed to get desktop path from DesktopChecker.");
            AfxMessageBox(L"Error: Could not retrieve desktop path.", MB_OK | MB_ICONERROR);
            AfxGetApp()->DoWaitCursor(-1);
            m_wndStatusBar.SetPaneText(0, L"Sort failed: No desktop path.");
            return;
        }
        LOG_DEBUG(L"CMainFrame::OnSortDesktop - Desktop path: " + desktopPath);

        std::vector<DesktopItem> items = dInfo.getAllDesktopItems(desktopPath);

        if (items.empty()) {
            LOG_INFO(L"CMainFrame::OnSortDesktop - No items found on the desktop.");
            AfxMessageBox(L"No items found on the desktop to sort.", MB_OK | MB_ICONINFORMATION);
            AfxGetApp()->DoWaitCursor(-1);
            m_wndStatusBar.SetPaneText(0, L"Sort complete: No items found.");
            return;
        }
        LOG_INFO(L"CMainFrame::OnSortDesktop - Found " + std::to_wstring(items.size()) + L" items. Classifying...");
        m_wndStatusBar.SetPaneText(0, (L"Classifying items (" + std::to_wstring(items.size()) + L" found)...").c_str());
        AfxGetApp()->PumpMessages();

        for (DesktopItem& item : items) { // Pass by non-const reference
            dSorter.classifyItem(item); // classifyItem now takes non-const DesktopItem&
        }

        LOG_INFO(L"CMainFrame::OnSortDesktop - Classification complete. Calculating layout...");
        m_wndStatusBar.SetPaneText(0, L"Calculating new layout...");
        AfxGetApp()->PumpMessages();

        layoutManager.calculateLayout(items, screenMetrics);

        LOG_INFO(L"CMainFrame::OnSortDesktop - Layout calculation complete. Applying new positions...");
        m_wndStatusBar.SetPaneText(0, L"Applying new layout...");
        AfxGetApp()->PumpMessages();

        for (const DesktopItem& item : items) {
            if (item.x != -1 && item.y != -1) {
                dSorter.setItemPosition(item.name, item.x, item.y);
            } else {
                LOG_DEBUG(L"CMainFrame::OnSortDesktop - Skipping item '" + item.name + L"' as it has no valid layout position (x or y is -1).");
            }
        }

        LOG_INFO(L"CMainFrame::OnSortDesktop - Desktop sorting process complete.");
        AfxMessageBox(L"Desktop sorting complete.", MB_OK | MB_ICONINFORMATION);

    } catch (const std::exception& e) {
        LOG_ERROR(L"CMainFrame::OnSortDesktop - Standard exception caught: " + CString(e.what()));
        AfxMessageBox(CString("An error occurred during sorting: ") + CString(e.what()), MB_OK | MB_ICONERROR);
    } catch (...) {
        LOG_ERROR(L"CMainFrame::OnSortDesktop - Unknown exception caught.");
        AfxMessageBox(L"An unknown error occurred during sorting.", MB_OK | MB_ICONERROR);
    }

    AfxGetApp()->DoWaitCursor(-1); // Restore cursor
    m_wndStatusBar.SetPaneText(0, L"Desktop sorting finished.");
}

void CMainFrame::OnSettings() {
    LOG_INFO(L"CMainFrame::OnSettings - Button clicked.");

    CSettingsDlg dlg(this); // Pass parent window
    dlg.DoModal();

    m_wndStatusBar.SetPaneText(0, L"Настройки закрыты."); // Updated status bar text
}
