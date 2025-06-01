#ifndef DESKTOPCHECKER_H
#define DESKTOPCHECKER_H

#include <vector>
#include <string>
// No need to include ConfigManager.h or Logging.h here if not directly used by declarations

class DesktopChecker {
public:
    std::vector<std::wstring> findInvalidShortcuts(const std::wstring& desktopPath);
    std::vector<std::wstring> findEmptyFolders(const std::wstring& desktopPath);
    std::wstring getDesktopPath();

private:
#ifdef _WIN32 // isLinkValid, isFolderEmpty, isSpecialSystemFolder are Windows-specific impl
    bool isLinkValid(const std::wstring& shortcutPath);
    bool isFolderEmpty(const std::wstring& folderPath);
    bool isSpecialSystemFolder(const std::wstring& folderPath);
#else // Stubs for non-Windows
    bool isLinkValid(const std::wstring& shortcutPath) { (void)shortcutPath; return false; }
    bool isFolderEmpty(const std::wstring& folderPath) { (void)folderPath; return true; } // Assume empty for stub
    bool isSpecialSystemFolder(const std::wstring& folderPath) { (void)folderPath; return false; }
#endif
};

#endif // DESKTOPCHECKER_H
