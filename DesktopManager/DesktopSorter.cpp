#include "DesktopSorter.h"
#include "../WebClassifier/WebClassifier.h"
#include "../Logging/Logging.h"          // For LOG_ macros
#include <algorithm>
#include <cctype>
#include <memory>
#include <string> // For std::to_wstring if not pulled by iostream/algorithm indirectly

#ifdef _WIN32
#include <windows.h>
#include <CommCtrl.h>
#include <ShlObj_core.h>
#include <comdef.h>
#pragma comment(lib, "Comctl32.lib")
#else
#include <iostream>
#endif

static std::wstring toLower_wstring_internal_ds(std::wstring s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](wchar_t c){ return std::towlower(c); });
    return s;
}

// Helper to convert ItemCategory to wstring for logging within DesktopSorter
static std::wstring itemCategoryToWString_ds(ItemCategory category) {
    switch (category) {
        case ItemCategory::FOLDER: return L"Folder";
        case ItemCategory::GAME: return L"Game";
        case ItemCategory::PROGRAM: return L"Program";
        case ItemCategory::TEXT_FILE: return L"Text File";
        case ItemCategory::IMAGE_FILE: return L"Image File";
        case ItemCategory::VIDEO_FILE: return L"Video File";
        case ItemCategory::ARCHIVE_FILE: return L"Archive File";
        case ItemCategory::UNCLASSIFIED: return L"Unclassified";
        case ItemCategory::SPECIAL_SYSTEM: return L"Special System";
        default: return L"Unknown Category";
    }
}


DesktopSorter::DesktopSorter()
    : m_webClassifier(std::make_unique<WebClassifier>()) {
    LOG_DEBUG(L"DesktopSorter::DesktopSorter() - Constructor called.");
}

DesktopSorter::~DesktopSorter() {
    LOG_DEBUG(L"DesktopSorter::~DesktopSorter() - Destructor called.");
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
    LOG_DEBUG(L"DesktopSorter::setItemPosition - Attempting to move item '" + itemName + L"' to (" + std::to_wstring(x) + L"," + std::to_wstring(y) + L")");
    HWND listViewHwnd = getDesktopListViewHandle();
    if (!listViewHwnd) {
        LOG_ERROR(L"DesktopSorter::setItemPosition - Failed to get Desktop ListView handle.");
        return;
    }
    int itemCount = ListView_GetItemCount(listViewHwnd);
    if (itemCount <= 0) return;
    DWORD dwProcessId;
    GetWindowThreadProcessId(listViewHwnd, &dwProcessId);
    HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);
    if (!hProcess) {
        LOG_ERROR(L"DesktopSorter::setItemPosition - OpenProcess failed. Error: " + std::to_wstring(GetLastError()));
        return;
    }
    int itemIndex = -1;
    for (int i = 0; i < itemCount; ++i) {
        if (getItemNameFromListView(listViewHwnd, i, hProcess) == itemName) {
            itemIndex = i;
            break;
        }
    }
    CloseHandle(hProcess);
    if (itemIndex != -1) {
        if(!ListView_SetItemPosition(listViewHwnd, itemIndex, x, y)){
            LOG_ERROR(L"DesktopSorter::setItemPosition - ListView_SetItemPosition failed. Error: " + std::to_wstring(GetLastError()));
        } else {
            LOG_INFO(L"DesktopSorter::setItemPosition - Successfully moved item '" + itemName + L"'.");
        }
    } else {
        LOG_WARNING(L"DesktopSorter::setItemPosition - Item '" + itemName + L"' not found in ListView.");
    }
#else
    LOG_DEBUG(L"DesktopSorter::setItemPosition (stub) - Item: " + itemName + L", Pos: (" + std::to_wstring(x) + L"," + std::to_wstring(y) + L")");
    (void)itemName; (void)x; (void)y;
#endif
}

ItemCategory DesktopSorter::classifyItem(const DesktopItem& item) {
    LOG_DEBUG(L"DesktopSorter::classifyItem - Attempting to classify item: " + item.name);

    if (item.type == DesktopItem::ItemType::FOLDER) {
        LOG_INFO(L"DesktopSorter::classifyItem - Item '" + item.name + L"' classified as FOLDER by type.");
        return ItemCategory::FOLDER;
    }

    std::wstring lowerName = toLower_wstring_internal_ds(item.name);
    std::wstring extension;
    size_t dotPos = lowerName.rfind(L'.');
    if (dotPos != std::wstring::npos && dotPos < lowerName.length() - 1) {
        extension = lowerName.substr(dotPos + 1);
    }

    if (extension == L"txt" || extension == L"log" || extension == L"md" || extension == L"doc" || extension == L"docx" || extension == L"pdf") {
        LOG_INFO(L"DesktopSorter::classifyItem - Item '" + item.name + L"' classified as TEXT_FILE by extension.");
        return ItemCategory::TEXT_FILE;
    }
    if (extension == L"png" || extension == L"jpg" || extension == L"jpeg" || extension == L"gif" || extension == L"bmp" || extension == L"svg") {
        LOG_INFO(L"DesktopSorter::classifyItem - Item '" + item.name + L"' classified as IMAGE_FILE by extension.");
        return ItemCategory::IMAGE_FILE;
    }
    if (extension == L"mp4" || extension == L"mkv" || extension == L"avi" || extension == L"mov" || extension == L"wmv") {
        LOG_INFO(L"DesktopSorter::classifyItem - Item '" + item.name + L"' classified as VIDEO_FILE by extension.");
        return ItemCategory::VIDEO_FILE;
    }
    if (extension == L"zip" || extension == L"rar" || extension == L"7z" || extension == L"tar" || extension == L"gz") {
        LOG_INFO(L"DesktopSorter::classifyItem - Item '" + item.name + L"' classified as ARCHIVE_FILE by extension.");
        return ItemCategory::ARCHIVE_FILE;
    }

#ifdef _WIN32
    if (m_webClassifier && (item.type == DesktopItem::ItemType::SHORTCUT ||
        (item.type == DesktopItem::ItemType::FILE && extension == L"exe"))) {

        LOG_DEBUG(L"DesktopSorter::classifyItem - Invoking WebClassifier for: " + item.name);
        std::string htmlDebugOutput;
        ItemCategory webCategory = m_webClassifier->performSearch(item, htmlDebugOutput);

        if (webCategory == ItemCategory::GAME || webCategory == ItemCategory::PROGRAM) {
            LOG_INFO(L"DesktopSorter::classifyItem - Item '" + item.name + L"' classified as " + itemCategoryToWString_ds(webCategory) + L" by WebClassifier."); // Corrected
            return webCategory;
        }
        LOG_INFO(L"DesktopSorter::classifyItem - WebClassifier returned UNCLASSIFIED for: " + item.name + L". Falling back to basic rules.");
    }
#endif

    if (item.type == DesktopItem::ItemType::SHORTCUT || extension == L"exe" || extension == L"app" || extension == L"bat" || extension == L"sh") {
        if (lowerName.find(L"game") != std::wstring::npos ||
            lowerName.find(L"игра") != std::wstring::npos ||
            lowerName.find(L"play") != std::wstring::npos ||
            lowerName.find(L"steam") != std::wstring::npos) {
            LOG_INFO(L"DesktopSorter::classifyItem - Item '" + item.name + L"' classified as GAME by basic name rules.");
            return ItemCategory::GAME;
        }
        if (lowerName.find(L"editor") != std::wstring::npos ||
            lowerName.find(L"browser") != std::wstring::npos ||
            lowerName.find(L"office") != std::wstring::npos ||
            lowerName.find(L"player") != std::wstring::npos ||
            lowerName.find(L"manager") != std::wstring::npos ||
            lowerName.find(L"studio") != std::wstring::npos ||
            lowerName.find(L"client") != std::wstring::npos) {
            LOG_INFO(L"DesktopSorter::classifyItem - Item '" + item.name + L"' classified as PROGRAM by basic name rules.");
            return ItemCategory::PROGRAM;
        }
        if (extension == L"exe" || item.type == DesktopItem::ItemType::SHORTCUT) {
            LOG_INFO(L"DesktopSorter::classifyItem - Item '" + item.name + L"' classified as PROGRAM by type (exe/shortcut).");
            return ItemCategory::PROGRAM;
        }
    }

    if (lowerName == L"recycle bin" || lowerName == L"корзина" ||
        lowerName == L"this pc" || lowerName == L"мой компьютер" ||
        lowerName == L"computer" ||
        lowerName == L"network" || lowerName == L"сеть") {
        LOG_INFO(L"DesktopSorter::classifyItem - Item '" + item.name + L"' classified as SPECIAL_SYSTEM by name.");
        return ItemCategory::SPECIAL_SYSTEM;
    }

    LOG_INFO(L"DesktopSorter::classifyItem - Item '" + item.name + L"' remains UNCLASSIFIED after all rules.");
    return ItemCategory::UNCLASSIFIED;
}
