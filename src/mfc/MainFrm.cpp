#include "MainFrm.h"
#include "../Logging/Logging.h" // Include Logging header

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
    AfxMessageBox(L"Кнопка 'Проверить рабочий стол' нажата!", MB_OK | MB_ICONINFORMATION);
    m_wndStatusBar.SetPaneText(0, L"Проверка рабочего стола...");
}

void CMainFrame::OnSortDesktop() {
    LOG_INFO(L"CMainFrame::OnSortDesktop - Button clicked.");
    AfxMessageBox(L"Кнопка 'Сортировать рабочий стол' нажата!", MB_OK | MB_ICONINFORMATION);
    m_wndStatusBar.SetPaneText(0, L"Сортировка рабочего стола...");
}

void CMainFrame::OnSettings() {
    LOG_INFO(L"CMainFrame::OnSettings - Button clicked.");
    AfxMessageBox(L"Кнопка 'Настройки' нажата!", MB_OK | MB_ICONINFORMATION);
    m_wndStatusBar.SetPaneText(0, L"Открытие настроек...");
}
