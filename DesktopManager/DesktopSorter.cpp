#include "DesktopSorter.h"
#include <algorithm> // For std::transform for case-insensitive compare
#include <cctype>    // For std::tolower

#ifdef _WIN32
#include <windows.h>
#include <CommCtrl.h>    // For ListView messages (needs Comctl32.lib)
#include <ShlObj_core.h> // For various shell functions
#include <comdef.h>      // For _com_error for debugging COM errors

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Ole32.lib") // For COM
#else
// Stubs for non-Windows
#include <iostream> // For stub messages
#endif

DesktopSorter::DesktopSorter() : comInitialized(false) {
#ifdef _WIN32
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr)) {
        comInitialized = true;
    } else {
        // std::wcerr << L"DesktopSorter: CoInitializeEx failed: " << _com_error(hr).ErrorMessage() << std::endl;
    }
#endif
}

DesktopSorter::~DesktopSorter() {
#ifdef _WIN32
    if (comInitialized) {
        CoUninitialize();
    }
#endif
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
                if (p) {
                    *reinterpret_cast<HWND*>(lParam) = p;
                    return FALSE;
                }
                return TRUE;
            }, reinterpret_cast<LPARAM>(&shellDllDefViewHwnd));
        }
    }
    if (!shellDllDefViewHwnd) return NULL;

    return FindWindowEx(shellDllDefViewHwnd, NULL, L"SysListView32", L"FolderView");
}

// Re-implement or adapt from DesktopInfo, ensuring it's available
std::wstring DesktopSorter::getItemNameFromListView(HWND listViewHwnd, int index, HANDLE hProcess) {
    const int bufferSize = MAX_PATH + 1;
    LVITEMW lvItem = {0};
    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = index;
    lvItem.iSubItem = 0;
    lvItem.cchTextMax = bufferSize;

    LPVOID remoteBuffer = VirtualAllocEx(hProcess, NULL, bufferSize * sizeof(wchar_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteBuffer) {
        return L"";
    }
    lvItem.pszText = (LPWSTR)remoteBuffer;

    if (SendMessage(listViewHwnd, LVM_GETITEMTEXTW, (WPARAM)index, (LPARAM)&lvItem) == 0) {
        // As before, 0 might be valid. ReadProcessMemory check is more crucial.
    }

    wchar_t localBuffer[bufferSize] = {0};
    SIZE_T bytesRead = 0;
    ReadProcessMemory(hProcess, remoteBuffer, localBuffer, bufferSize * sizeof(wchar_t), &bytesRead);

    VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
    return std::wstring(localBuffer);
}
#endif


void DesktopSorter::setItemPosition(const std::wstring& itemName, int x, int y) {
#ifdef _WIN32
    if (!comInitialized) {
        // std::wcerr << L"COM not initialized in DesktopSorter." << std::endl;
        return;
    }

    HWND listViewHwnd = getDesktopListViewHandle();
    if (!listViewHwnd) {
        // std::wcerr << L"Desktop ListView handle not found for setItemPosition." << std::endl;
        return;
    }

    int itemCount = ListView_GetItemCount(listViewHwnd);
    if (itemCount <= 0) {
        return;
    }

    DWORD dwProcessId;
    GetWindowThreadProcessId(listViewHwnd, &dwProcessId);
    HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);
    if (!hProcess) {
        // std::wcerr << L"Failed to open process for ListView for setItemPosition. Error: " << GetLastError() << std::endl;
        return;
    }

    int itemIndex = -1;
    for (int i = 0; i < itemCount; ++i) {
        std::wstring currentItemName = getItemNameFromListView(listViewHwnd, i, hProcess);
        if (currentItemName == itemName) {
            itemIndex = i;
            break;
        }
    }

    CloseHandle(hProcess); // Close process handle once done with reading names

    if (itemIndex != -1) {
        // Using SendMessage for LVM_SETITEMPOSITION. It might be better to use PostMessage
        // if the desktop ListView is busy, but SendMessage is simpler for a direct attempt.
        // LVM_SETITEMPOSITION expects coordinates relative to the ListView client area.
        if (!ListView_SetItemPosition(listViewHwnd, itemIndex, x, y)) {
            // std::wcerr << L"ListView_SetItemPosition failed for item: " << itemName << L". Error: " << GetLastError() << std::endl;
        } else {
            // std::wcout << L"Attempted to move " << itemName << L" to (" << x << L", " << y << L")" << std::endl;
            // It's also good to call ListView_Arrange(listViewHwnd, LVA_DEFAULT) if you want to snap to grid,
            // but that might override the specific X, Y you set if "Align to Grid" is off.
            // For now, direct placement is attempted. Persistence is another challenge (e.g. desktop.ini or registry).
        }
    } else {
        // std::wcerr << L"Item not found in ListView: " << itemName << std::endl;
    }
#else
    // Non-Windows stub
    // std::wcout << L"[Stub] Attempted to set position for: " << itemName << L" to (" << x << L", " << y << L")" << std::endl;
    (void)itemName; (void)x; (void)y; // Mark as unused
#endif
}

ItemCategory DesktopSorter::classifyItem(const DesktopItem& item) {
    // Basic placeholder classification
    if (item.type == DesktopItem::ItemType::FOLDER) {
        return ItemCategory::FOLDER;
    }

    // Convert name to lowercase for easier matching
    std::wstring lowerName = item.name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                   [](wchar_t c){ return std::tolower(c); });

    // File extensions
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


    if (item.type == DesktopItem::ItemType::SHORTCUT || extension == L"exe" || extension == L"app" || extension == L"bat" || extension == L"sh") {
        // Very naive keyword check for games
        if (lowerName.find(L"game") != std::wstring::npos ||
            lowerName.find(L"игра") != std::wstring::npos || // Russian for "game"
            lowerName.find(L"play") != std::wstring::npos ||
            // Add more game-related keywords if desired
            lowerName.find(L"steam") != std::wstring::npos) {
            return ItemCategory::GAME;
        }
        return ItemCategory::PROGRAM;
    }

    // Check for special system items by name (very basic)
    // This part needs to be robust, potentially using known folder IDs or system checks
    // For now, it's just a placeholder.
    if (lowerName == L"recycle bin" || lowerName == L"корзина" ||
        lowerName == L"this pc" || lowerName == L"мой компьютер" ||
        lowerName == L"computer" ||
        lowerName == L"network" || lowerName == L"сеть") {
        return ItemCategory::SPECIAL_SYSTEM;
    }


    return ItemCategory::UNCLASSIFIED;
}
