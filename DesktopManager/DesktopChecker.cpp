#include "DesktopChecker.h"
#include "../ConfigManager/ConfigManager.h"
#include "../Logging/Logging.h"

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <shlobj_core.h>
#include <pathcch.h>
#include <objbase.h>
#pragma comment(lib, "Pathcch.lib")
#pragma comment(lib, "Ole32.lib")
#else
#include <cstdlib>
#include <sys/stat.h>
#include <dirent.h>
#include <locale>
#include <codecvt>
#include <algorithm>
#include <cstring>   // For strlen (used in logging an exception's e.what())
#include <filesystem> // For std::filesystem::path in stubs for more robust checks
#endif

// Non-Windows string conversion stubs (if not already in a common utility)
#ifndef _WIN32
static std::string wstring_to_utf8_string_stub_dc(const std::wstring& wstr) {
    // (Implementation from previous step)
    std::string s = "";
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        s = converter.to_bytes(wstr);
    } catch (const std::exception& e) {
        for (wchar_t wc : wstr) { if (wc >= 0 && wc < 128) s += static_cast<char>(wc); else s += '?'; }
        LOG_ERROR(L"wstring_to_utf8_string_stub_dc failed: " + std::wstring(e.what(), e.what() + strlen(e.what())));
    }
    return s;
}
static std::wstring utf8_string_to_wstring_stub_dc(const std::string& str) {
    // (Implementation from previous step)
    std::wstring ws = L"";
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        ws = converter.from_bytes(str);
    } catch (const std::exception& e) {
        for (char c : str) { ws += static_cast<wchar_t>(static_cast<unsigned char>(c));  }
        LOG_ERROR(L"utf8_string_to_wstring_stub_dc failed: " + std::wstring(e.what(), e.what() + strlen(e.what())));
    }
    return ws;
}
#endif


std::wstring DesktopChecker::getDesktopPath() {
#ifdef _WIN32
    wchar_t desktopPath_buf[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath_buf))) {
        return std::wstring(desktopPath_buf);
    }
    LOG_ERROR(L"DesktopChecker::getDesktopPath: SHGetFolderPathW failed. Error: " + std::to_wstring(GetLastError()));
    return L"";
#else
    const char* homeDir = getenv("HOME");
    if (homeDir) {
        try {
            std::filesystem::path desktopFsPath = std::filesystem::path(homeDir) / "Desktop";
            return desktopFsPath.wstring(); // Uses std::filesystem for path construction
        } catch (const std::exception &e) {
            LOG_ERROR(L"DesktopChecker::getDesktopPath (stub): Filesystem error constructing path: " + std::wstring(e.what(), e.what() + strlen(e.what())));
            return L"";
        }
    }
    LOG_ERROR(L"DesktopChecker::getDesktopPath (stub): Failed to get HOME environment variable.");
    return L"";
#endif
}

#ifdef _WIN32
bool DesktopChecker::isLinkValid(const std::wstring& shortcutPath) {
    // (Implementation from previous step, COM init/uninit, IShellLink, etc.)
    // Ensure CoInitialize/CoUninitialize are balanced or handled by a RAII wrapper.
    HRESULT hr_com = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    IShellLink* psl = NULL;
    bool isValid = false;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if (SUCCEEDED(hr)) {
        IPersistFile* ppf = NULL;
        hr = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
        if (SUCCEEDED(hr)) {
            hr = ppf->Load(shortcutPath.c_str(), STGM_READ);
            if (SUCCEEDED(hr)) {
                hr = psl->Resolve(NULL, SLR_NO_UI | SLR_NOUPDATE | SLR_NOLINKINFO);
                if (SUCCEEDED(hr)) {
                    wchar_t targetPath[MAX_PATH];
                    hr = psl->GetPath(targetPath, MAX_PATH, NULL, SLGP_UNCPRIORITY);
                    if (SUCCEEDED(hr) && targetPath[0] != L'\0') {
                        if (PathFileExistsW(targetPath)) {
                            isValid = true;
                        } else {
                             LOG_DEBUG(L"DesktopChecker::isLinkValid: Shortcut target does not exist: " + std::wstring(targetPath) + L" (Link: " + shortcutPath + L")");
                        }
                    } // else GetPath failed or empty
                } // else Resolve failed - link is broken
            } // else Load failed
            if(ppf) ppf->Release();
        } // else QueryInterface failed
        if(psl) psl->Release();
    } else { LOG_ERROR(L"DesktopChecker::isLinkValid: CoCreateInstance(CLSID_ShellLink) failed. Error: " + std::to_wstring(hr)); }
    if(SUCCEEDED(hr_com)) CoUninitialize(); // Only uninitialize if this call initialized
    return isValid;
}

bool DesktopChecker::isFolderEmpty(const std::wstring& folderPath) {
    // (Implementation from previous step, FindFirstFileW/FindNextFileW)
    std::wstring searchPath = folderPath + L"\\*";
    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        LOG_ERROR(L"DesktopChecker::isFolderEmpty: FindFirstFileW failed for folder: " + folderPath + L". Error: " + std::to_wstring(GetLastError()));
        return false;
    }
    do {
        if (wcscmp(findFileData.cFileName, L".") != 0 && wcscmp(findFileData.cFileName, L"..") != 0) {
            FindClose(hFind); return false;
        }
    } while (FindNextFileW(hFind, &findFileData) != 0);
    DWORD lastError = GetLastError();
    FindClose(hFind);
    if (lastError != ERROR_NO_MORE_FILES) {
        LOG_WARNING(L"DesktopChecker::isFolderEmpty: Error iterating folder '" + folderPath + L"'. FindNextFileW ended with error: " + std::to_wstring(lastError));
        return false;
    }
    return true;
}

bool DesktopChecker::isSpecialSystemFolder(const std::wstring& folderPath) {
    // (Implementation from previous step)
    std::wstring folderName = L"";
    try { folderName = std::filesystem::path(folderPath).filename().wstring(); } catch(...) {} // Robustness
    std::wstring lowerFolderName = folderName;
    std::transform(lowerFolderName.begin(), lowerFolderName.end(), lowerFolderName.begin(), ::towlower);
    const static std::vector<std::wstring> specialNames = { L"recycle bin", L"корзина", L"system volume information", L"desktop.ini", L"thumbs.db", L"$recycle.bin" };
    for(const auto& special : specialNames) if (lowerFolderName == special) return true;
    return false;
}
#endif

std::vector<std::wstring> DesktopChecker::findInvalidShortcuts(const std::wstring& desktopPath) {
    LOG_INFO(L"DesktopChecker::findInvalidShortcuts - Starting scan on: " + desktopPath);
    std::vector<std::wstring> invalidShortcuts;
    const auto& excludedPaths = ConfigManager::getInstance().getExcludedPaths();

#ifdef _WIN32
    std::wstring searchPath = desktopPath + L"\\*.lnk";
    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        LOG_ERROR(L"DesktopChecker::findInvalidShortcuts: FindFirstFileW failed for path: " + searchPath + L". Error: " + std::to_wstring(GetLastError()));
        return invalidShortcuts;
    }
    do {
        std::wstring shortcutFileName = findFileData.cFileName;
        std::wstring currentItemFullPath = desktopPath + L"\\" + shortcutFileName;

        // For robust path comparison, consider normalizing paths first.
        // e.g. auto normalizedItemPath = std::filesystem::path(currentItemFullPath).lexically_normal();
        //      auto normalizedExcludedPath = std::filesystem::path(excludedPath).lexically_normal();
        //      if (normalizedItemPath.wstring().rfind(normalizedExcludedPath.wstring(), 0) == 0) { ... }
        bool isExcluded = false;
        for (const auto& excludedPath : excludedPaths) {
            if (currentItemFullPath.rfind(excludedPath, 0) == 0) { // Simple starts-with check
                isExcluded = true;
                break;
            }
        }
        if (isExcluded) {
            LOG_INFO(L"DesktopChecker::findInvalidShortcuts - Skipping excluded item: " + currentItemFullPath);
            continue;
        }

        // Proceed with .lnk check only if not excluded
        // (Original code had .lnk check as part of searchPath, which is more efficient)
        // The prompt implies checking all files then filtering .lnk, but FindFirstFileW already filters.
        // The exclusion should be before isLinkValid.

        if (!isLinkValid(currentItemFullPath)) {
            LOG_INFO(L"DesktopChecker::findInvalidShortcuts - Found invalid shortcut: " + currentItemFullPath);
            invalidShortcuts.push_back(currentItemFullPath);
        }
    } while (FindNextFileW(hFind, &findFileData) != 0);
    FindClose(hFind);
#else
    // LOG_DEBUG(L"DesktopChecker::findInvalidShortcuts: Using non-Windows stub for path: " + desktopPath);
#endif
    LOG_INFO(L"DesktopChecker::findInvalidShortcuts - Scan complete. Found " + std::to_wstring(invalidShortcuts.size()) + L" invalid shortcuts.");
    return invalidShortcuts;
}

std::vector<std::wstring> DesktopChecker::findEmptyFolders(const std::wstring& desktopPath) {
    LOG_INFO(L"DesktopChecker::findEmptyFolders - Starting scan on: " + desktopPath);
    std::vector<std::wstring> emptyFolders;
    const auto& excludedPaths = ConfigManager::getInstance().getExcludedPaths();

#ifdef _WIN32
    std::wstring searchPath = desktopPath + L"\\*"; // Iterate all items
    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
         LOG_ERROR(L"DesktopChecker::findEmptyFolders: FindFirstFileW failed for path: " + searchPath + L". Error: " + std::to_wstring(GetLastError()));
        return emptyFolders;
    }
    do {
        std::wstring currentItemFullPath = desktopPath + L"\\" + findFileData.cFileName;

        // For robust path comparison, consider normalizing paths first.
        bool isExcluded = false;
        for (const auto& excludedPath : excludedPaths) {
            if (currentItemFullPath.rfind(excludedPath, 0) == 0) { // Simple starts-with check
                isExcluded = true;
                break;
            }
        }
        if (isExcluded) {
            LOG_INFO(L"DesktopChecker::findEmptyFolders - Skipping excluded item: " + currentItemFullPath);
            continue;
        }

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (wcscmp(findFileData.cFileName, L".") != 0 && wcscmp(findFileData.cFileName, L"..") != 0) {
                // currentItemFullPath is already constructed
                if (isSpecialSystemFolder(currentItemFullPath)) {
                    LOG_DEBUG(L"DesktopChecker::findEmptyFolders - Skipping special system folder: " + currentItemFullPath);
                    continue;
                }
                if (isFolderEmpty(currentItemFullPath)) {
                    LOG_INFO(L"DesktopChecker::findEmptyFolders - Found empty folder: " + currentItemFullPath);
                    emptyFolders.push_back(currentItemFullPath);
                }
            }
        }
    } while (FindNextFileW(hFind, &findFileData) != 0);
    FindClose(hFind);
#else
    // LOG_DEBUG(L"DesktopChecker::findEmptyFolders: Using non-Windows stub for path: " + desktopPath);
    try {
        for (const auto& entry : std::filesystem::directory_iterator(wstring_to_utf8_string_stub_dc(desktopPath))) {
            if (entry.is_directory()) { // Check if it's a directory
                std::wstring fullFolderPath = utf8_string_to_wstring_stub_dc(entry.path().string());

                bool isExcluded = false;
                for (const auto& excludedPath : excludedPaths) {
                    if (fullFolderPath.rfind(excludedPath, 0) == 0) {
                        LOG_INFO(L"DesktopChecker::findEmptyFolders (stub) - Skipping excluded folder: " + fullFolderPath);
                        isExcluded = true;
                        break;
                    }
                }
                if (isExcluded) continue;

                if (isSpecialSystemFolder(fullFolderPath)) continue;

                std::error_code ec;
                if (std::filesystem::is_empty(entry.path(), ec) && !ec) { // Check if directory is empty
                     LOG_INFO(L"DesktopChecker::findEmptyFolders (stub) - Found empty folder: " + fullFolderPath);
                    emptyFolders.push_back(fullFolderPath);
                } else if (ec) {
                    LOG_ERROR(L"DesktopChecker::findEmptyFolders (stub) - Error checking if folder is empty '" + fullFolderPath + L"': " + std::wstring(ec.message().begin(), ec.message().end()));
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR(L"DesktopChecker::findEmptyFolders (stub) - Error iterating directory '" + desktopPath + L"': " + std::wstring(e.what(), e.what() + strlen(e.what())));
    }
#endif
    LOG_INFO(L"DesktopChecker::findEmptyFolders - Scan complete. Found " + std::to_wstring(emptyFolders.size()) + L" empty folders.");
    return emptyFolders;
}
