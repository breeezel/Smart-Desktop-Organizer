#ifndef DESKTOPCHECKER_H
#define DESKTOPCHECKER_H

#include <vector>
#include <string>

class DesktopChecker {
public:
    std::vector<std::wstring> findInvalidShortcuts(const std::wstring& desktopPath);
    std::vector<std::wstring> findEmptyFolders(const std::wstring& desktopPath);
    std::wstring getDesktopPath(); // Moved to public for now for testing in main

private:
    bool isLinkValid(const std::wstring& shortcutPath);
    bool isFolderEmpty(const std::wstring& folderPath);
    bool isSpecialSystemFolder(const std::wstring& folderPath);
    // std::wstring getDesktopPath(); // Original position
};

#endif // DESKTOPCHECKER_H
