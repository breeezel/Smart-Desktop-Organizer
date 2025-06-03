#pragma once
#include <afxwin.h>
#include <afxext.h>
#include <afxbutton.h>
#include "SettingsDlg.h" // Added for CSettingsDlg

// Define resource IDs
#define IDR_MAINFRAME                   128
#define IDC_BUTTON_CHECK_DESKTOP        1001
#define IDC_BUTTON_SORT_DESKTOP         1002
#define IDC_BUTTON_SETTINGS             1003
// Example indicator ID for status bar (if used more complexly)
// #define ID_INDICATOR_PANE1              1004

class CMainFrame : public CFrameWnd {
public:
    CMainFrame();
    virtual ~CMainFrame();

#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    CStatusBar  m_wndStatusBar;
    CButton m_btnCheckDesktop;
    CButton m_btnSortDesktop;
    CButton m_btnSettings;

protected:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnCheckDesktop();
    afx_msg void OnSortDesktop();
    afx_msg void OnSettings();
    DECLARE_MESSAGE_MAP()
};
