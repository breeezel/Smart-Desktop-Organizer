#include "DesktopSorter.h"
#include "../WebClassifier/WebClassifier.h" // Include full definition here
#include <algorithm>
#include <cctype>
#include <memory> // For std::make_unique

#ifdef _WIN32
#include <windows.h>
#include <CommCtrl.h>
#include <ShlObj_core.h>
#include <comdef.h>
#pragma comment(lib, "Comctl32.lib")
#else
#include <iostream>
#endif

static std::wstring toLower_wstring_internal(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](wchar_t c){ return std::towlower(c); });
    return s;
}
static std::string toLower_string_internal(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}

DesktopSorter::DesktopSorter()
    : m_webClassifier(std::make_unique<WebClassifier>()) {
    // m_webClassifier is initialized
}

DesktopSorter::~DesktopSorter() {
    // Destructor definition is needed here for std::unique_ptr to forward-declared type.
    // The default implementation is fine.
}

#ifdef _WIN32
HWND DesktopSorter::getDesktopListViewHandle() {
    HWND desktopHwnd = GetDesktopWindow();
    if (!desktopHwnd) return NULL;
    HWND shellDllDefViewHwnd = FindWindowEx(desktopHwnd, NULL, L"SHELLDLL_DefView", NULL);
    if (!shellDllDefViewHwnd) {
        HWND progmanHwnd = FindWindow(L"Progman", NULL);
        if (progmanHwnd) {
            shellDllDefViewHwnd = FindWindowEx(progmanHwnd, NULL, L"SHELLDLL_DefView", NULL);
        }
        if (!shellDllDefViewHwnd) {
            EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
                HWND p = FindWindowEx(hwnd, NULL, L"SHELLDLL_DefView", NULL);
                if (p) { *reinterpret_cast<HWND*>(lParam) = p; return FALSE; }
                return TRUE;
            }, reinterpret_cast<LPARAM>(&shellDllDefViewHwnd));
        }
    }
    if (!shellDllDefViewHwnd) return NULL;
    return FindWindowEx(shellDllDefViewHwnd, NULL, L"SysListView32", L"FolderView");
}

std::wstring DesktopSorter::getItemNameFromListView(HWND listViewHwnd, int index, HANDLE hProcess) {
    const int bufferSize = MAX_PATH + 1;
    LVITEMW lvItem = {0};
    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = index;
    lvItem.iSubItem = 0;
    lvItem.cchTextMax = bufferSize;
    LPVOID remoteBuffer = VirtualAllocEx(hProcess, NULL, bufferSize * sizeof(wchar_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteBuffer) return L"";
    lvItem.pszText = (LPWSTR)remoteBuffer;
    SendMessage(listViewHwnd, LVM_GETITEMTEXTW, (WPARAM)index, (LPARAM)&lvItem);
    wchar_t localBuffer[bufferSize] = {0};
    ReadProcessMemory(hProcess, remoteBuffer, localBuffer, bufferSize * sizeof(wchar_t), NULL);
    VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
    return std::wstring(localBuffer);
}
#endif

void DesktopSorter::setItemPosition(const std::wstring& itemName, int x, int y) {
#ifdef _WIN32
    HWND listViewHwnd = getDesktopListViewHandle();
    if (!listViewHwnd) return;
    int itemCount = ListView_GetItemCount(listViewHwnd);
    if (itemCount <= 0) return;

    DWORD dwProcessId;
    GetWindowThreadProcessId(listViewHwnd, &dwProcessId);
    HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);
    if (!hProcess) return;

    int itemIndex = -1;
    for (int i = 0; i < itemCount; ++i) {
        if (getItemNameFromListView(listViewHwnd, i, hProcess) == itemName) {
            itemIndex = i;
            break;
        }
    }
    CloseHandle(hProcess);
    if (itemIndex != -1) {
        ListView_SetItemPosition(listViewHwnd, itemIndex, x, y);
    }
#else
    (void)itemName; (void)x; (void)y;
#endif
}

ItemCategory DesktopSorter::classifyItem(const DesktopItem& item) {
    if (item.type == DesktopItem::ItemType::FOLDER) {
        return ItemCategory::FOLDER;
    }
    std::wstring lowerName = toLower_wstring_internal(item.name);
    std::wstring extension;
    size_t dotPos = lowerName.rfind(L'.');
    if (dotPos != std::wstring::npos && dotPos < lowerName.length() - 1) {
        extension = lowerName.substr(dotPos + 1);
    }

    if (extension == L"txt" || extension == L"log" || extension == L"md" || extension == L"doc" || extension == L"docx" || extension == L"pdf") {
        return ItemCategory::TEXT_FILE;
    }
    if (extension == L"png" || extension == L"jpg" || extension == L"jpeg" || extension == L"gif" || extension == L"bmp" || extension == L"svg") {
        return ItemCategory::IMAGE_FILE;
    }
    if (extension == L"mp4" || extension == L"mkv" || extension == L"avi" || extension == L"mov" || extension == L"wmv") {
        return ItemCategory::VIDEO_FILE;
    }
    if (extension == L"zip" || extension == L"rar" || extension == L"7z" || extension == L"tar" || extension == L"gz") {
        return ItemCategory::ARCHIVE_FILE;
    }

#ifdef _WIN32
    if (m_webClassifier && (item.type == DesktopItem::ItemType::SHORTCUT ||
        (item.type == DesktopItem::ItemType::FILE && extension == L"exe"))) {

        std::string htmlDebugOutput;
        ItemCategory webCategory = m_webClassifier->performSearch(item, htmlDebugOutput); // Use -> with unique_ptr

        if (webCategory == ItemCategory::GAME || webCategory == ItemCategory::PROGRAM) {
            return webCategory;
        }
    }
#endif

    if (item.type == DesktopItem::ItemType::SHORTCUT || extension == L"exe" || extension == L"app" || extension == L"bat" || extension == L"sh") {
        if (lowerName.find(L"game") != std::wstring::npos ||
            lowerName.find(L"игра") != std::wstring::npos ||
            lowerName.find(L"play") != std::wstring::npos ||
            lowerName.find(L"steam") != std::wstring::npos) {
            return ItemCategory::GAME;
        }
        if (lowerName.find(L"editor") != std::wstring::npos ||
            lowerName.find(L"browser") != std::wstring::npos ||
            lowerName.find(L"office") != std::wstring::npos ||
            lowerName.find(L"player") != std::wstring::npos ||
            lowerName.find(L"manager") != std::wstring::npos ||
            lowerName.find(L"studio") != std::wstring::npos ||
            lowerName.find(L"client") != std::wstring::npos) {
            return ItemCategory::PROGRAM;
        }
        if (extension == L"exe" || item.type == DesktopItem::ItemType::SHORTCUT) {
            return ItemCategory::PROGRAM;
        }
    }

    if (lowerName == L"recycle bin" || lowerName == L"корзина" ||
        lowerName == L"this pc" || lowerName == L"мой компьютер" ||
        lowerName == L"computer" ||
        lowerName == L"network" || lowerName == L"сеть") {
        return ItemCategory::SPECIAL_SYSTEM;
    }

    return ItemCategory::UNCLASSIFIED;
}
