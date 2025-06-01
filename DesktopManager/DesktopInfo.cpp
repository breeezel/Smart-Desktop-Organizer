#include "DesktopInfo.h"
#include <iostream> // For debug output

#ifdef _WIN32
#include <windows.h>
#include <ShellScalingApi.h> // For GetDpiForMonitor (needs Shcore.lib)
#include <CommCtrl.h>        // For ListView messages (needs Comctl32.lib)
#include <ShlObj_core.h>     // For SHGetFolderPathW, etc.
#include <oleacc.h>          // For IAccessible
#include <comdef.h>          // For _com_error

#pragma comment(lib, "Shcore.lib")   // For GetDpiForMonitor
#pragma comment(lib, "Comctl32.lib") // For ListView controls
#pragma comment(lib, "Ole32.lib")    // For COM
#else
// Stubs for non-Windows
#include <filesystem>   // For std::filesystem
#include <cstdlib>      // For getenv (used in DesktopChecker, maybe useful here too)
#include <sys/stat.h>   // For struct stat, S_ISDIR, S_ISREG (needed by determineItemType if not using filesystem status)
#include <string>       // For std::string
#include <vector>       // For std::vector
// For wstring/string conversion if std::wstring_convert is problematic
#include <locale>
#include <codecvt>
#endif

DesktopInfo::DesktopInfo() : comInitialized(false) {
#ifdef _WIN32
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr)) {
        comInitialized = true;
    } else {
        // std::wcerr << L"DesktopInfo: CoInitializeEx failed: " << _com_error(hr).ErrorMessage() << std::endl;
    }
#endif
}

DesktopInfo::~DesktopInfo() {
#ifdef _WIN32
    if (comInitialized) {
        CoUninitialize();
    }
#endif
}

int DesktopInfo::getScreenWidth() {
#ifdef _WIN32
    return GetSystemMetrics(SM_CXSCREEN);
#else
    return 1920; // Stub
#endif
}

int DesktopInfo::getScreenHeight() {
#ifdef _WIN32
    return GetSystemMetrics(SM_CYSCREEN);
#else
    return 1080; // Stub
#endif
}

float DesktopInfo::getPrimaryMonitorDpiScale() {
#ifdef _WIN32
    HMONITOR hMonitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
    if (hMonitor) {
        UINT dpiX, dpiY;
        HRESULT hr = GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
        if (SUCCEEDED(hr)) {
            return static_cast<float>(dpiX) / 96.0f;
        } else {
            return 1.0f;
        }
    }
    return 1.0f;
#else
    return 1.0f; // Stub
#endif
}

#ifndef _WIN32
// Helper function for basic wstring to string conversion (UTF-8) for non-Windows stubs
std::string wstring_to_utf8_string_stub(const std::wstring& wstr) {
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(wstr);
    } catch (const std::exception& e) {
        std::string s = "";
        for (wchar_t wc : wstr) {
            if (wc >= 0 && wc < 128) s += static_cast<char>(wc);
            else s += '?';
        }
        return s;
    }
}

std::wstring utf8_string_to_wstring_stub(const std::string& str) {
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(str);
    } catch (const std::exception& e) {
        std::wstring ws = L"";
        for (char c : str) {
            ws += static_cast<wchar_t>(static_cast<unsigned char>(c)); // cast to unsigned char first
        }
        return ws;
    }
}
#endif


#ifdef _WIN32
HWND DesktopInfo::getDesktopListViewHandle() {
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

std::wstring DesktopInfo::getItemNameFromListView(HWND listViewHwnd, int index, HANDLE hProcess) {
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
    SIZE_T bytesRead = 0;
    ReadProcessMemory(hProcess, remoteBuffer, localBuffer, bufferSize * sizeof(wchar_t), &bytesRead);
    VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
    return std::wstring(localBuffer);
}

POINT DesktopInfo::getItemPositionFromListView(HWND listViewHwnd, int index) {
    POINT pt = {0, 0};
    ListView_GetItemPosition(listViewHwnd, index, &pt);
    return pt;
}
#endif

#ifdef _WIN32
DesktopItem::ItemType DesktopInfo::determineItemType(const std::wstring& itemPath, DWORD fileAttributes) {
    if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        return DesktopItem::ItemType::FOLDER;
    }
    if (itemPath.length() > 4 && itemPath.substr(itemPath.length() - 4) == L".lnk") {
        return DesktopItem::ItemType::SHORTCUT;
    }
    return DesktopItem::ItemType::FILE;
}
#else
// Non-Windows version
DesktopItem::ItemType DesktopInfo::determineItemType(const std::wstring& itemPath, unsigned int fileAttributes /*unused*/) {
    (void)fileAttributes; // Mark as unused for clarity
    std::string path_s = wstring_to_utf8_string_stub(itemPath);
    struct stat sb;
    if (stat(path_s.c_str(), &sb) == 0) {
        if (S_ISDIR(sb.st_mode)) return DesktopItem::ItemType::FOLDER;
        if (S_ISREG(sb.st_mode)) {
            // Basic check for .desktop files as "shortcuts" on Linux
            if (itemPath.length() > 8 && itemPath.substr(itemPath.length() - 8) == L".desktop") {
                 return DesktopItem::ItemType::SHORTCUT;
            }
            return DesktopItem::ItemType::FILE;
        }
    }
    return DesktopItem::ItemType::OTHER;
}
#endif

bool DesktopInfo::getFileLastModified(const std::wstring& itemPath, std::filesystem::file_time_type& lastModifiedTime) {
    try {
        std::error_code ec;
        lastModifiedTime = std::filesystem::last_write_time(itemPath, ec);
        if (ec) {
            // std::wcerr << L"Failed to get last modified time for " << itemPath << L": " << ec.message() << std::endl;
            lastModifiedTime = std::filesystem::file_time_type::min();
            return false;
        }
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        // std::wcerr << L"Exception: Failed to get last modified time for " << itemPath << L": " << e.what() << std::endl;
        lastModifiedTime = std::filesystem::file_time_type::min();
        return false;
    }
}

std::vector<DesktopItem> DesktopInfo::getAllDesktopItems(const std::wstring& desktopPath) {
    std::vector<DesktopItem> items;
#ifdef _WIN32
    HWND listViewHwnd = getDesktopListViewHandle();
    if (!listViewHwnd) {
        WIN32_FIND_DATAW findFileData;
        std::wstring searchPath = desktopPath + L"\\*.*";
        HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findFileData);
        if (hFind == INVALID_HANDLE_VALUE) return items;
        do {
            if (wcscmp(findFileData.cFileName, L".") != 0 && wcscmp(findFileData.cFileName, L"..") != 0) {
                DesktopItem item;
                item.name = findFileData.cFileName;
                item.path = desktopPath + L"\\" + item.name;
                item.type = determineItemType(item.path, findFileData.dwFileAttributes);
                item.x = -1;
                item.y = -1;
                getFileLastModified(item.path, item.lastModified);
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
    if (!hProcess) return items;

    for (int i = 0; i < itemCount; ++i) {
        DesktopItem item;
        item.name = getItemNameFromListView(listViewHwnd, i, hProcess);
        if (item.name.empty()) continue;
        item.path = desktopPath + L"\\" + item.name;
        POINT pt = getItemPositionFromListView(listViewHwnd, i);
        item.x = pt.x;
        item.y = pt.y;
        WIN32_FILE_ATTRIBUTE_DATA fileAttributesData;
        if (GetFileAttributesExW(item.path.c_str(), GetFileExInfoStandard, &fileAttributesData)) {
            item.type = determineItemType(item.path, fileAttributesData.dwFileAttributes);
            ULARGE_INTEGER ull;
            ull.LowPart = fileAttributesData.ftLastWriteTime.dwLowDateTime;
            ull.HighPart = fileAttributesData.ftLastWriteTime.dwHighDateTime;
            // Formula to convert FILETIME to file_time_type
            item.lastModified = std::filesystem::file_time_type(std::chrono::nanoseconds(ull.QuadPart * 100 - 116444736000000000LL));
        } else {
            item.type = DesktopItem::ItemType::OTHER;
            item.lastModified = std::filesystem::file_time_type::min();
        }
        items.push_back(item);
    }
    CloseHandle(hProcess);

#else
    // Non-Windows stub: List files in desktopPath
    if (desktopPath.empty()) return items;

    try {
        // Convert wstring desktopPath to string for std::filesystem on Linux/macOS
        std::string path_s = wstring_to_utf8_string_stub(desktopPath);
        if (!std::filesystem::exists(path_s) || !std::filesystem::is_directory(path_s)) {
            // std::wcerr << L"Desktop path (stub) does not exist or is not a directory: " << desktopPath << std::endl;
            return items;
        }

        for (const auto& entry : std::filesystem::directory_iterator(path_s)) {
            DesktopItem item;
            // Use helper for path conversion
            item.path = utf8_string_to_wstring_stub(entry.path().string());
            item.name = utf8_string_to_wstring_stub(entry.path().filename().string());

            std::error_code ec;
            auto status = entry.status(ec);
            if(ec) continue;

            if (std::filesystem::is_directory(status)) {
                item.type = DesktopItem::ItemType::FOLDER;
            } else if (std::filesystem::is_regular_file(status)) {
                 if (item.path.length() > 8 && item.path.substr(item.path.length() - 8) == L".desktop") {
                     item.type = DesktopItem::ItemType::SHORTCUT;
                 } else {
                    item.type = DesktopItem::ItemType::FILE;
                 }
            } else {
                item.type = DesktopItem::ItemType::OTHER;
            }
            item.x = -1;
            item.y = -1;
            getFileLastModified(item.path, item.lastModified); // item.path is wstring
            items.push_back(item);
        }
    } catch (const std::filesystem::filesystem_error& e) {
        // std::wcerr << L"Error iterating desktop path (stub): " << utf8_string_to_wstring_stub(e.what()) << std::endl;
    }
#endif
    return items;
}
