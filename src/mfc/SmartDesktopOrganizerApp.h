#pragma once
#include <afxwin.h> // Main MFC header

// Forward declaration
class CMainFrame;

class CSmartDesktopOrganizerApp : public CWinApp {
public:
    CSmartDesktopOrganizerApp();
    virtual BOOL InitInstance();
    // virtual int ExitInstance();

protected:
    DECLARE_MESSAGE_MAP()
};

extern CSmartDesktopOrganizerApp theApp; // Global app object
