#include "DesktopInfo.h"
#include <iostream> // For debug output
#include "../Logging/Logging.h" // For LOG_ macros

#ifdef _WIN32
#include <windows.h>
#include <ShellScalingApi.h>
#include <CommCtrl.h>
#include <ShlObj_core.h>
#include <oleacc.h>
#include <comdef.h>

#pragma comment(lib, "Shcore.lib")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Ole32.lib")
#else
#include <filesystem>
#include <cstdlib>
#include <sys/stat.h>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>
#include <cstring> // For strlen in logging e.what()
#endif

DesktopInfo::DesktopInfo() {
#ifdef _WIN32
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        LOG_ERROR(L"DesktopInfo: CoInitializeEx failed in constructor. Error: " + std::to_wstring(hr));
    }
#endif
}

DesktopInfo::~DesktopInfo() {
#ifdef _WIN32
    CoUninitialize();
#endif
}

int DesktopInfo::getScreenWidth() { /* ... as before ... */
#ifdef _WIN32
    return GetSystemMetrics(SM_CXSCREEN);
#else
    return 1920;
#endif
}
int DesktopInfo::getScreenHeight() { /* ... as before ... */
#ifdef _WIN32
    return GetSystemMetrics(SM_CYSCREEN);
#else
    return 1080;
#endif
}
float DesktopInfo::getPrimaryMonitorDpiScale() { /* ... as before ... */
#ifdef _WIN32
    HMONITOR hMonitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
    if (hMonitor) {
        UINT dpiX, dpiY;
        HRESULT hr = GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
        if (SUCCEEDED(hr)) {
            return static_cast<float>(dpiX) / 96.0f;
        } else { return 1.0f; }
    }
    return 1.0f;
#else
    return 1.0f;
#endif
}

#ifndef _WIN32
std::string wstring_to_utf8_string_stub_di(const std::wstring& wstr) { // Renamed for DI
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(wstr);
    } catch (const std::exception& e) {
        std::string s = "";
        for (wchar_t wc : wstr) { if (wc >= 0 && wc < 128) s += static_cast<char>(wc); else s += '?'; }
        LOG_ERROR(L"wstring_to_utf8_string_stub_di failed: " + std::wstring(e.what(), e.what() + strlen(e.what())));
        return s;
    }
}
std::wstring utf8_string_to_wstring_stub_di(const std::string& str) { // Renamed for DI
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(str);
    } catch (const std::exception& e) {
        std::wstring ws = L"";
        for (char c : str) { ws += static_cast<wchar_t>(static_cast<unsigned char>(c)); }
        LOG_ERROR(L"utf8_string_to_wstring_stub_di failed: " + std::wstring(e.what(), e.what() + strlen(e.what())));
        return ws;
    }
}
#endif


#ifdef _WIN32
HWND DesktopInfo::getDesktopListViewHandle() { /* ... as before ... */
    HWND desktopHwnd = GetDesktopWindow();
    if (!desktopHwnd) return NULL;
    HWND shellDllDefViewHwnd = FindWindowEx(desktopHwnd, NULL, L"SHELLDLL_DefView", NULL);
    if (!shellDllDefViewHwnd) {
        HWND progmanHwnd = FindWindow(L"Progman", NULL);
        if (progmanHwnd) { shellDllDefViewHwnd = FindWindowEx(progmanHwnd, NULL, L"SHELLDLL_DefView", NULL); }
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
std::wstring DesktopInfo::getItemNameFromListView(HWND listViewHwnd, int index, HANDLE hProcess) { /* ... as before ... */
    const int bufferSize = MAX_PATH + 1; LVITEMW lvItem = {0}; lvItem.mask = LVIF_TEXT; lvItem.iItem = index; lvItem.iSubItem = 0; lvItem.cchTextMax = bufferSize;
    LPVOID remoteBuffer = VirtualAllocEx(hProcess, NULL, bufferSize * sizeof(wchar_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteBuffer) return L""; lvItem.pszText = (LPWSTR)remoteBuffer; SendMessage(listViewHwnd, LVM_GETITEMTEXTW, (WPARAM)index, (LPARAM)&lvItem);
    wchar_t localBuffer[bufferSize] = {0}; SIZE_T bytesRead = 0; ReadProcessMemory(hProcess, remoteBuffer, localBuffer, bufferSize * sizeof(wchar_t), &bytesRead);
    VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE); return std::wstring(localBuffer);
}
POINT DesktopInfo::getItemPositionFromListView(HWND listViewHwnd, int index) { /* ... as before ... */
    POINT pt = {0, 0}; ListView_GetItemPosition(listViewHwnd, index, &pt); return pt;
}
#endif

#ifdef _WIN32
DesktopItem::ItemType DesktopInfo::determineItemType(const std::wstring& itemPath, DWORD fileAttributes) { /* ... */
    if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY) return DesktopItem::ItemType::FOLDER;
    if (itemPath.length() > 4 && itemPath.substr(itemPath.length() - 4) == L".lnk") return DesktopItem::ItemType::SHORTCUT;
    return DesktopItem::ItemType::FILE;
}
#else
DesktopItem::ItemType DesktopInfo::determineItemType(const std::wstring& itemPath, unsigned int fileAttributes) { /* ... */
    (void)fileAttributes;
    std::string path_s = wstring_to_utf8_string_stub_di(itemPath); // Use renamed stub
    struct stat sb;
    if (stat(path_s.c_str(), &sb) == 0) {
        if (S_ISDIR(sb.st_mode)) return DesktopItem::ItemType::FOLDER;
        if (S_ISREG(sb.st_mode)) {
            if (itemPath.length() > 8 && itemPath.substr(itemPath.length() - 8) == L".desktop") return DesktopItem::ItemType::SHORTCUT;
            return DesktopItem::ItemType::FILE;
        }
    }
    return DesktopItem::ItemType::OTHER;
}
#endif

bool DesktopInfo::getFileLastModified(const std::wstring& itemPath, std::filesystem::file_time_type& lastModifiedTime) { /* ... */
    try {
        std::error_code ec; lastModifiedTime = std::filesystem::last_write_time(itemPath, ec);
        if (ec) { lastModifiedTime = std::filesystem::file_time_type::min(); return false; }
        return true;
    } catch (const std::filesystem::filesystem_error&) {
        lastModifiedTime = std::filesystem::file_time_type::min(); return false;
    }
}

std::vector<DesktopItem> DesktopInfo::getAllDesktopItems(const std::wstring& desktopPath) {
    std::vector<DesktopItem> items;
#ifdef _WIN32
    HWND listViewHwnd = getDesktopListViewHandle();
    if (!listViewHwnd) {
        LOG_WARNING(L"DesktopInfo::getAllDesktopItems - Failed to get Desktop ListView handle. Falling back to directory scan.");
        WIN32_FIND_DATAW findFileData;
        std::wstring searchPath = desktopPath + L"\\*.*";
        HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findFileData);
        if (hFind == INVALID_HANDLE_VALUE) {
            LOG_ERROR(L"DesktopInfo::getAllDesktopItems - Fallback FindFirstFileW failed. Error: " + std::to_wstring(GetLastError()));
            return items;
        }
        do {
            if (wcscmp(findFileData.cFileName, L".") != 0 && wcscmp(findFileData.cFileName, L"..") != 0) {
                DesktopItem item;
                item.name = findFileData.cFileName;
                item.path = desktopPath + L"\\" + item.name;
                item.type = determineItemType(item.path, findFileData.dwFileAttributes);
                item.original_x = -1; // Coordinates unknown from this method
                item.original_y = -1;
                item.x = -1;          // Initialize target positions
                item.y = -1;
                getFileLastModified(item.path, item.lastModified);
                item.category = ItemCategory::UNCLASSIFIED; // Default category
                items.push_back(item);
            }
        } while (FindNextFileW(hFind, &findFileData) != 0);
        FindClose(hFind);
        return items;
    }

    int itemCount = ListView_GetItemCount(listViewHwnd);
    if (itemCount <= 0) return items;

    DWORD dwProcessId;
    GetWindowThreadProcessId(listViewHwnd, &dwProcessId);
    HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);
    if (!hProcess) {
        LOG_ERROR(L"DesktopInfo::getAllDesktopItems - OpenProcess failed. Error: " + std::to_wstring(GetLastError()));
        return items;
    }

    for (int i = 0; i < itemCount; ++i) {
        DesktopItem item;
        item.name = getItemNameFromListView(listViewHwnd, i, hProcess);
        if (item.name.empty()) continue;
        item.path = desktopPath + L"\\" + item.name;

        POINT pt = getItemPositionFromListView(listViewHwnd, i);
        item.original_x = pt.x; // Store original position
        item.original_y = pt.y;
        item.x = -1;          // Initialize target positions to indicate they are not yet set by layout manager
        item.y = -1;

        WIN32_FILE_ATTRIBUTE_DATA fileAttributesData;
        if (GetFileAttributesExW(item.path.c_str(), GetFileExInfoStandard, &fileAttributesData)) {
            item.type = determineItemType(item.path, fileAttributesData.dwFileAttributes);
            ULARGE_INTEGER ull;
            ull.LowPart = fileAttributesData.ftLastWriteTime.dwLowDateTime;
            ull.HighPart = fileAttributesData.ftLastWriteTime.dwHighDateTime;
            item.lastModified = std::filesystem::file_time_type(std::chrono::nanoseconds(ull.QuadPart * 100 - 116444736000000000LL));
        } else {
            LOG_WARNING(L"DesktopInfo::getAllDesktopItems - GetFileAttributesExW failed for: " + item.path + L". Error: " + std::to_wstring(GetLastError()));
            item.type = DesktopItem::ItemType::OTHER;
            item.lastModified = std::filesystem::file_time_type::min();
        }
        item.category = ItemCategory::UNCLASSIFIED; // Default category, to be set by Sorter
        items.push_back(item);
    }
    CloseHandle(hProcess);

#else
    // Non-Windows stub
    if (desktopPath.empty()) return items;
    try {
        std::string path_s = wstring_to_utf8_string_stub_di(desktopPath); // Use renamed stub
        if (!std::filesystem::exists(path_s) || !std::filesystem::is_directory(path_s)) {
            LOG_WARNING(L"DesktopInfo::getAllDesktopItems (stub) - Desktop path does not exist or is not a directory: " + desktopPath);
            return items;
        }
        for (const auto& entry : std::filesystem::directory_iterator(path_s)) {
            DesktopItem item;
            item.path = utf8_string_to_wstring_stub_di(entry.path().string()); // Use renamed stub
            item.name = utf8_string_to_wstring_stub_di(entry.path().filename().string()); // Use renamed stub
            std::error_code ec;
            auto status = entry.status(ec);
            if(ec) { LOG_WARNING(L"DesktopInfo::getAllDesktopItems (stub) - Error getting status for " + item.path + L": " + std::wstring(ec.message().begin(), ec.message().end())); continue; }

            if (std::filesystem::is_directory(status)) {
                item.type = DesktopItem::ItemType::FOLDER;
            } else if (std::filesystem::is_regular_file(status)) {
                 if (item.path.length() > 8 && item.path.substr(item.path.length() - 8) == L".desktop") {
                     item.type = DesktopItem::ItemType::SHORTCUT;
                 } else { item.type = DesktopItem::ItemType::FILE; }
            } else { item.type = DesktopItem::ItemType::OTHER; }

            item.original_x = -1;
            item.original_y = -1;
            item.x = -1;
            item.y = -1;
            getFileLastModified(item.path, item.lastModified);
            item.category = ItemCategory::UNCLASSIFIED;
            items.push_back(item);
        }
    } catch (const std::exception& e) {
        LOG_ERROR(L"DesktopInfo::getAllDesktopItems (stub) - Error iterating directory '" + desktopPath + L"': " + std::wstring(e.what(), e.what() + strlen(e.what())));
    }
#endif
    return items;
}
