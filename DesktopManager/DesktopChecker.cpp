#include "DesktopChecker.h"
#include <iostream>
#include <vector>
#include <string>

// Conditional includes for Windows
#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>      // For SHGetFolderPathW
#include <shlobj_core.h> // For IShellLink
#include <pathcch.h>     // For PathFileExistsW (PathCch.lib) or Shlwapi.h (Shlwapi.lib) for PathFileExists
#include <objbase.h>     // For CoInitialize, CoUninitialize, CoCreateInstance

// Link with necessary libraries for MSVC
#pragma comment(lib, "Pathcch.lib") // Or "Shlwapi.lib" if using that for PathFileExists
#pragma comment(lib, "Ole32.lib")   // For COM functions
#else
// Includes for basic Linux/macOS stubs
#include <cstdlib>      // For getenv
#include <sys/stat.h>   // For stat (checking file/dir type and existence)
#include <dirent.h>     // For opendir, readdir, closedir
#include <locale>       // For std::locale, std::use_facet, std::ctype
#include <codecvt>      // For std::codecvt_utf8_utf16 (may still be problematic)
#endif

// Helper function for basic wstring to string conversion (UTF-8) for non-Windows stubs
#ifndef _WIN32
std::string wstring_to_utf8_string(const std::wstring& wstr) {
    // This is a common point of failure with std::wstring_convert in C++17
    // For stubbing, a simpler approach might be needed if this fails,
    // or ensure the compiler/stdlib supports it.
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(wstr);
    } catch (const std::exception& e) {
        // Fallback: very basic conversion (likely lossy, only good for ASCII)
        std::string s = "";
        for (wchar_t wc : wstr) {
            if (wc < 128) s += static_cast<char>(wc);
            else s += '?'; // Placeholder for non-ASCII
        }
        return s;
    }
}

std::wstring utf8_string_to_wstring(const std::string& str) {
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(str);
    } catch (const std::exception& e) {
        std::wstring ws = L"";
        for (char c : str) {
            ws += static_cast<wchar_t>(c); // Very basic, lossy
        }
        return ws;
    }
}
#endif


std::wstring DesktopChecker::getDesktopPath() {
#ifdef _WIN32
    wchar_t desktopPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath))) {
        return std::wstring(desktopPath);
    }
    return L"";
#else
    const char* homeDir = getenv("HOME");
    if (homeDir) {
        std::string desktopPathStr = std::string(homeDir) + "/Desktop";
        return utf8_string_to_wstring(desktopPathStr);
    }
    return L"";
#endif
}

bool DesktopChecker::isLinkValid(const std::wstring& shortcutPath) {
#ifdef _WIN32
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        return false;
    }
    IShellLink* psl = NULL;
    bool isValid = false;
    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if (SUCCEEDED(hr)) {
        IPersistFile* ppf = NULL;
        hr = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
        if (SUCCEEDED(hr)) {
            hr = ppf->Load(shortcutPath.c_str(), STGM_READ);
            if (SUCCEEDED(hr)) {
                hr = psl->Resolve(NULL, SLR_NO_UI);
                if (SUCCEEDED(hr)) {
                    wchar_t targetPath[MAX_PATH];
                    hr = psl->GetPath(targetPath, MAX_PATH, NULL, SLGP_UNCPRIORITY);
                    if (SUCCEEDED(hr) && targetPath[0] != L'\0') {
                        if (PathFileExistsW(targetPath)) {
                            isValid = true;
                        }
                    }
                }
            }
            ppf->Release();
        }
        psl->Release();
    }
    CoUninitialize();
    return isValid;
#else
    (void)shortcutPath;
    return false;
#endif
}

bool DesktopChecker::isFolderEmpty(const std::wstring& folderPath) {
#ifdef _WIN32
    std::wstring searchPath = folderPath + L"\\*";
    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) return false;
    do {
        if (wcscmp(findFileData.cFileName, L".") != 0 && wcscmp(findFileData.cFileName, L"..") != 0) {
            FindClose(hFind);
            return false;
        }
    } while (FindNextFileW(hFind, &findFileData) != 0);
    FindClose(hFind);
    return GetLastError() == ERROR_NO_MORE_FILES;
#else
    std::string path_s = wstring_to_utf8_string(folderPath);
    DIR *dir = opendir(path_s.c_str());
    if (dir == NULL) return false;
    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (std::string(entry->d_name) != "." && std::string(entry->d_name) != "..") {
            count++;
            break;
        }
    }
    closedir(dir);
    return count == 0;
#endif
}

bool DesktopChecker::isSpecialSystemFolder(const std::wstring& folderPath) {
#ifdef _WIN32
    std::wstring folderName = folderPath.substr(folderPath.find_last_of(L"\\/") + 1);
    std::wstring lowerFolderName;
    lowerFolderName.resize(folderName.size());
    for(size_t i = 0; i < folderName.size(); ++i) lowerFolderName[i] = towlower(folderName[i]);
    if (lowerFolderName == L"recycle bin" || lowerFolderName == L"корзина" ||
        lowerFolderName == L"system volume information" ||
        lowerFolderName == L"desktop.ini" || lowerFolderName == L"thumbs.db") {
        return true;
    }
#else
    std::wstring folderName = folderPath.substr(folderPath.find_last_of(L"/") + 1);
    if (!folderName.empty() && folderName[0] == L'.') return true;
    if (folderName == L"Trash" || folderName == L".Trash" || folderName == L".local/share/Trash") return true;
#endif
    (void)folderPath;
    return false;
}

std::vector<std::wstring> DesktopChecker::findInvalidShortcuts(const std::wstring& desktopPath) {
    std::vector<std::wstring> invalidShortcuts;
#ifdef _WIN32
    std::wstring searchPath = desktopPath + L"\\*.lnk";
    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findFileData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::wstring fullShortcutPath = desktopPath + L"\\" + findFileData.cFileName;
            if (!isLinkValid(fullShortcutPath)) {
                invalidShortcuts.push_back(fullShortcutPath);
            }
        } while (FindNextFileW(hFind, &findFileData) != 0);
        FindClose(hFind);
    }
#else
    (void)desktopPath;
#endif
    return invalidShortcuts;
}

std::vector<std::wstring> DesktopChecker::findEmptyFolders(const std::wstring& desktopPath) {
    std::vector<std::wstring> emptyFolders;
    if (desktopPath.empty()) return emptyFolders;
#ifdef _WIN32
    std::wstring searchPath = desktopPath + L"\\*";
    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findFileData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (wcscmp(findFileData.cFileName, L".") != 0 && wcscmp(findFileData.cFileName, L"..") != 0) {
                    std::wstring fullFolderPath = desktopPath + L"\\" + findFileData.cFileName;
                    if (!isSpecialSystemFolder(fullFolderPath)) {
                        if (isFolderEmpty(fullFolderPath)) {
                            emptyFolders.push_back(fullFolderPath);
                        }
                    }
                }
            }
        } while (FindNextFileW(hFind, &findFileData) != 0);
        FindClose(hFind);
    }
#else
    std::string path_s = wstring_to_utf8_string(desktopPath);
    DIR *dir = opendir(path_s.c_str());
    if (dir == NULL) return emptyFolders;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name_s = entry->d_name;
        if (name_s != "." && name_s != "..") {
            std::string fullPath_s = path_s + "/" + name_s;
            struct stat statbuf;
            if (stat(fullPath_s.c_str(), &statbuf) == 0) {
                if (S_ISDIR(statbuf.st_mode)) {
                    std::wstring fullPath_ws = utf8_string_to_wstring(fullPath_s);
                    if (!isSpecialSystemFolder(fullPath_ws)) {
                        if (isFolderEmpty(fullPath_ws)) {
                            emptyFolders.push_back(fullPath_ws);
                        }
                    }
                }
            }
        }
    }
    closedir(dir);
#endif
    return emptyFolders;
}
