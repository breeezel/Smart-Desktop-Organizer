#include "stdafx.h" // Added for PCH
#include "SmartDesktopOrganizerApp.h"
#include "MainFrm.h"
#include "../Logging/Logging.h" // Include Logging header

CSmartDesktopOrganizerApp theApp;

BEGIN_MESSAGE_MAP(CSmartDesktopOrganizerApp, CWinApp)
END_MESSAGE_MAP()

CSmartDesktopOrganizerApp::CSmartDesktopOrganizerApp() {
    LOG_DEBUG(L"CSmartDesktopOrganizerApp::CSmartDesktopOrganizerApp() - Constructor called.");
}

BOOL CSmartDesktopOrganizerApp::InitInstance() {
    LOG_INFO(L"CSmartDesktopOrganizerApp::InitInstance - MFC App Initialization started.");

    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();
    LOG_DEBUG(L"CSmartDesktopOrganizerApp::InitInstance - CWinApp::InitInstance() called.");

    CMainFrame* pMainFrame = new CMainFrame;
    if (!pMainFrame) {
        LOG_ERROR(L"CSmartDesktopOrganizerApp::InitInstance - Failed to create CMainFrame object (new CMainFrame returned null).");
        return FALSE;
    }
    LOG_DEBUG(L"CSmartDesktopOrganizerApp::InitInstance - CMainFrame object created.");

    if (!pMainFrame->LoadFrame(IDR_MAINFRAME)) {
        LOG_ERROR(L"CSmartDesktopOrganizerApp::InitInstance - pMainFrame->LoadFrame(IDR_MAINFRAME) failed. Resources might be missing or SDO_メインフレーム ID is incorrect.");
        delete pMainFrame;
        return FALSE;
    }
    m_pMainWnd = pMainFrame;
    LOG_INFO(L"CSmartDesktopOrganizerApp::InitInstance - Main window loaded and set using LoadFrame.");

    pMainFrame->ShowWindow(m_nCmdShow);
    pMainFrame->UpdateWindow();
    LOG_INFO(L"CSmartDesktopOrganizerApp::InitInstance - Main window shown and updated.");

    return TRUE;
}
