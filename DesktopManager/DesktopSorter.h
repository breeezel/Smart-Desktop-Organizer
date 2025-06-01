#ifndef DESKTOPSORTER_H
#define DESKTOPSORTER_H

#include "DesktopInfo.h" // For DesktopItem
// #include "../WebClassifier/WebClassifier.h" // REMOVE full include
#include <string>
#include <vector>
#include <memory> // For std::unique_ptr

// Forward declaration to break include cycle
class WebClassifier;

// ItemCategory is defined here
enum class ItemCategory {
    FOLDER,
    GAME,
    PROGRAM,
    TEXT_FILE,
    IMAGE_FILE,
    VIDEO_FILE,
    ARCHIVE_FILE,
    UNCLASSIFIED,
    SPECIAL_SYSTEM
};

class DesktopSorter {
public:
    DesktopSorter();
    ~DesktopSorter(); // Required for std::unique_ptr with forward-declared type

    void setItemPosition(const std::wstring& itemName, int x, int y);
    ItemCategory classifyItem(const DesktopItem& item);

private:
    std::unique_ptr<WebClassifier> m_webClassifier; // Use unique_ptr

#ifdef _WIN32
    HWND getDesktopListViewHandle();
    std::wstring getItemNameFromListView(HWND listViewHwnd, int index, HANDLE hProcess);
#endif
};

#endif // DESKTOPSORTER_H
